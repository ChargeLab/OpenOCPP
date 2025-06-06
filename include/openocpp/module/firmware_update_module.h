#ifndef CHARGELAB_OPEN_FIRMWARE_FIRMWARE_UPDATE_MODULE_H
#define CHARGELAB_OPEN_FIRMWARE_FIRMWARE_UPDATE_MODULE_H

#include "openocpp/interface/component/system_interface.h"
#include "openocpp/interface/component/fetch_interface.h"
#include "openocpp/interface/platform_interface.h"
#include "openocpp/common/operation_holder.h"
#include "openocpp/module/common_templates.h"
#include "openocpp/module/pending_messages_module.h"
#include "openocpp/module/connector_status_module.h"
#include "openocpp/module/reset_module.h"
#include "openocpp/common/settings.h"
#include "openocpp/common/logging.h"
#include "openocpp/common/macro.h"

#include <thread>
#include <condition_variable>
#include <charconv>

namespace chargelab {
    namespace detail {
        template <typename HM>
        struct FirmwareUpdateOperation {
            ocpp2_0::UpdateFirmwareRequest request;

            // for ocpp1.6 only
            std::optional<ocpp1_6::FirmwareStatus> last_status1_6 = std::nullopt;

            // L01.FR.04
            std::unique_ptr<typename HM::hash_calculator_type> signature_hash = HM::createCalculator(HM::kHashTypeSHA256);
            std::shared_ptr<RestConnectionInterface> connection = nullptr;
            std::vector<uint8_t> buffer {};
            std::size_t content_length = 0;
            std::size_t total_bytes_read = 0;
            std::size_t total_failures = 0;

            std::vector<std::vector<uint8_t>> block_hashes {};
            std::optional<std::vector<uint8_t>> expected_signature_hash = std::nullopt;
            std::optional<ocpp2_0::FirmwareStatusEnumType> last_status2_0 = std::nullopt;

            bool finished_signature_check = false;
            bool finished_flashing_firmware = false;
            bool running_firmware_update = false;
        };
    }

    template <typename HM>
    class FirmwareUpdateModule : public ServiceStatefulGeneral {
    private:
        static constexpr int kChunkSize = 20*1024;
        static constexpr int kPriorityFirmwareStatusNotification = 100;

        // Note: arbitrary random assigned ID
        static constexpr std::uint64_t kOperationGroupId = 0x78106033AC8E780Aull;
    public:
        explicit FirmwareUpdateModule(
                std::shared_ptr<PlatformInterface> platform,
                std::shared_ptr<ResetModule> reset,
                std::shared_ptr<PendingMessagesModule> pending_messages,
                std::shared_ptr<ConnectorStatusModule> connector_status,
                std::shared_ptr<StationInterface> station
        )
            : platform_(std::move(platform)),
              reset_(std::move(reset)),
              pending_messages_(std::move(pending_messages)),
              connector_status_(std::move(connector_status)),
              station_(std::move(station))
        {
            assert(platform_ != nullptr);
            assert(station_ != nullptr);

            settings_ = platform_->getSettings();
            assert(settings_ != nullptr);
        }

        ~FirmwareUpdateModule() override {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting FirmwareUpdateModule";
        }

    private:
        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::UpdateFirmwareRsp>>
        onUpdateFirmwareReq(const ocpp1_6::UpdateFirmwareReq &req) override {
            if (operation_.has_value()) {
                return ocpp1_6::CallError {
                        ocpp1_6::ErrorCode::kGenericError,
                        "InProgress",
                        common::RawJson::empty_object()
                };
            }

            // Check if there is transaction in progress
            bool found_charging_connector = false;
            for (auto const& entry: station_->getConnectorMetadata()) {
                if (entry.first.id == 0) continue;
                auto const connector_status = station_->pollConnectorStatus(entry.first);
                if (!connector_status.has_value())
                    continue;
                if (connector_status->charging_enabled) {
                    found_charging_connector = true;
                    break;
                }
            }

            if (found_charging_connector) {
                return ocpp1_6::CallError {
                        ocpp1_6::ErrorCode::kGenericError,
                        "ActiveChargeSession",
                        common::RawJson::empty_object()
                };
            }

            // Set all connectors to Unavailable
            need_to_restore_connector_0_operative_ = connector_status_->setConnector0Inoperative(true);

            operation_ = detail::FirmwareUpdateOperation<HM> {
                ocpp2_0::UpdateFirmwareRequest {
                    req.retries,
                    req.retryInterval,
                    0,
                    ocpp2_0::FirmwareType {
                        req.location.value(),
                        optional::GetOrDefault(req.retrieveDate.getTimestamp(), platform_->systemClockNow())
                    }
                }
            };

            return ocpp1_6::UpdateFirmwareRsp {};
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::UpdateFirmwareResponse>>
        onUpdateFirmwareReq(const ocpp2_0::UpdateFirmwareRequest &request) override {
            // Note: there doesn't appear to be any specific requirement here in the 2.0.1 specification and this is
            // incompatible with a station simultaneously supporting L02...but this is required by TC_L_18_CS which is
            // mandatory for core certification.
            if (!request.firmware.signature.has_value()) {
                return ocpp2_0::UpdateFirmwareResponse{
                    ocpp2_0::UpdateFirmwareStatusEnumType::kRejected,
                    ocpp2_0::StatusInfoType {"MissingSignature"}
                };
            }
            if (!request.firmware.signingCertificate.has_value()) {
                return ocpp2_0::UpdateFirmwareResponse{
                        ocpp2_0::UpdateFirmwareStatusEnumType::kRejected,
                        ocpp2_0::StatusInfoType {"MissingSigningCert"}
                };
            }

            // L01.FR.21
            // L01.FR.22
            if (request.firmware.signingCertificate.has_value()) {
                if (!platform_->verifyManufacturerCertificate(request.firmware.signingCertificate->value(), std::nullopt)) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Bad manufacturer certificate";

                    // L01.FR.02
                    pending_messages_->sendRequest2_0(
                            ocpp2_0::SecurityEventNotificationRequest {
                                    "InvalidFirmwareSigningCertificate",
                                    platform_->systemClockNow()
                            },
                            buildPendingMessagePolicy(PendingMessageType::kSecurityEvent)
                    );

                    return ocpp2_0::UpdateFirmwareResponse{ocpp2_0::UpdateFirmwareStatusEnumType::kInvalidCertificate};
                }
            }

            // Note: this isn't strictly necessary, however passing a free-form text string to the platform URI parsing
            // method seems potentially dangerous.
            if (!uri::parseHttpUri(request.firmware.location.value()).has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad firmware location: " << request.firmware.location.value();
                return ocpp2_0::UpdateFirmwareResponse{ocpp2_0::UpdateFirmwareStatusEnumType::kRejected};
            }

            auto status = ocpp2_0::UpdateFirmwareStatusEnumType::kAccepted;
            if (operation_.has_value()) {
                // L01.FR.24
                operation_ = std::nullopt;
                status = ocpp2_0::UpdateFirmwareStatusEnumType::kAcceptedCanceled;
            }

            operation_ = detail::FirmwareUpdateOperation<HM>{request};
            return ocpp2_0::UpdateFirmwareResponse{status};
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::TriggerMessageRsp>>
        onTriggerMessageReq(const ocpp1_6::TriggerMessageReq&) override {
            return std::nullopt;
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::TriggerMessageResponse>>
        onTriggerMessageReq(const ocpp2_0::TriggerMessageRequest &req) override {
            if (req.requestedMessage != ocpp2_0::MessageTriggerEnumType::kFirmwareStatusNotification)
                return std::nullopt;

            if (req.evse.has_value()) {
                force_update_ = false;
            } else {
                force_update_ = true;
            }

            return ocpp2_0::TriggerMessageResponse {ocpp2_0::TriggerMessageStatusEnumType::kAccepted};
        }

        void checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType status) {
            if (operation_->last_status2_0 == std::make_optional(status))
                return;

            pending_messages_->sendRequest2_0(
                    ocpp2_0::FirmwareStatusNotificationRequest {
                        status,
                        operation_->request.requestId
                    },
                    buildPendingMessagePolicy(PendingMessageType::kNotificationEvent)
            );

            operation_->last_status2_0 = status;
        }

        void checkAndUpdateStatus(ocpp1_6::FirmwareStatus status) {
            if (operation_->last_status1_6 == std::make_optional(status))
                return;

            pending_messages_->sendRequest1_6(
                    ocpp1_6::FirmwareStatusNotificationReq {
                            status
                    },
                    buildPendingMessagePolicy(PendingMessageType::kNotificationEvent)
            );

            operation_->last_status1_6 = status;
        }


        static bool checkOrRetryConnection(
                std::shared_ptr<PlatformInterface> const& platform,
                detail::FirmwareUpdateOperation<HM>& operation
        ) {
            if (operation.connection != nullptr)
                return true;

            auto const uri = operation.request.firmware.location.value();
            CHARGELAB_LOG_MESSAGE(info) << "Downloading firmware from: " << uri;
            operation.connection = platform->getRequest(uri);
            if (operation.connection == nullptr) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed establishing connection to: " << uri;
                operation.total_failures++;
                return false;
            }

            if (!operation.connection->open(0)) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed opening connection to server";
                operation.total_failures++;
                operation.connection = nullptr;
                return false;
            }

            if (!operation.connection->send()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed sending request";
                operation.total_failures++;
                operation.connection = nullptr;
                return false;
            }

            auto const status = operation.connection->getStatusCode();
            if (!(status >= 200 && status < 300)) {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad status code - expecting 2xx response: " << status;
                operation.total_failures++;
                operation.connection = nullptr;
                return false;
            }

            operation.content_length = operation.connection->getContentLength();
            operation.total_bytes_read = 0;
            return true;
        }

        void runStep(ocpp1_6::OcppRemote &) override {
            performBootChecks1_6();
            performFirmwareUpdate1_6();
        }

        void runStep(ocpp2_0::OcppRemote& remote) override {
            performBootChecks2_0();
            performFirmwareUpdate2_0();

            if (force_update_.has_value()) {
                if (!force_update_.value()) {
                    // Note - doesn't seem to be defined by 2.0.1 requirements, following TC_F_17_CS
                    auto const result = remote.sendCall(ocpp2_0::FirmwareStatusNotificationRequest {
                        ocpp2_0::FirmwareStatusEnumType::kIdle
                    });

                    if (result.has_value())
                        force_update_ = std::nullopt;
                } else {
                    if (operation_.has_value() && operation_->last_status2_0.has_value()) {
                        auto const result = remote.sendCall(ocpp2_0::FirmwareStatusNotificationRequest {
                                operation_->last_status2_0.value(),
                                operation_->request.requestId
                        });

                        if (result.has_value())
                            force_update_ = std::nullopt;
                    } else {
                        // F06.FR.16
                        auto const result = remote.sendCall(ocpp2_0::FirmwareStatusNotificationRequest {
                                ocpp2_0::FirmwareStatusEnumType::kIdle
                        });

                        if (result.has_value())
                            force_update_ = std::nullopt;
                    }
                }
            }
        }

        void performBootChecks2_0() {
            if (performed_boot_checks_)
                return;

            auto const active = station_->getActiveSlotId();
            settings_->ActiveFirmwareSlotId.setValue(active);

            auto const update = settings_->ExpectedUpdateFirmwareSlotId.getValue();
            if (!update.empty()) {
                int request_id;
                std::string slot_id;

                parseExpectedUpdateFirmwareSlotId(update, request_id, slot_id);

                pending_messages_->sendRequest2_0(
                        ocpp2_0::FirmwareStatusNotificationRequest {
                                slot_id == active ?
                                ocpp2_0::FirmwareStatusEnumType::kInstalled :
                                ocpp2_0::FirmwareStatusEnumType::kInstallationFailed,
                                request_id
                        },
                        buildPendingMessagePolicy(PendingMessageType::kNotificationEvent)
                );

                pending_messages_->sendRequest2_0(
                        ocpp2_0::SecurityEventNotificationRequest{
                                "FirmwareUpdated",
                                platform_->systemClockNow()
                        },
                        buildPendingMessagePolicy(PendingMessageType::kSecurityEvent)
                );

                settings_->ExpectedUpdateFirmwareSlotId.setValue("");
            }

            performed_boot_checks_ = true;
        }

        void performFirmwareUpdate2_0() {
            if (!operation_.has_value())
                return;

            auto const max_retries = optional::GetOrDefault(
                    operation_->request.retries,
                    settings_->FirmwareUpdateDefaultRetries.getValue()
            );

            if (operation_->request.firmware.retrieveDateTime.isAfter(platform_->systemClockNow(), false)) {
                checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kDownloadScheduled);
                return;
            }

            if (!operation_->finished_signature_check) {
                if ((int)operation_->total_failures > max_retries) {
                    checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kDownloadFailed);
                    operation_ = std::nullopt;
                    return;
                }

                checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kDownloading);
                if (!checkOrRetryConnection(platform_, operation_.value()))
                    return;

                auto const remaining = (int)operation_->content_length - (int)operation_->total_bytes_read;
                if (remaining > 0) {
                    if (operation_->total_bytes_read == 0)
                        operation_->signature_hash->reset();

                    operation_->buffer.resize(kChunkSize);
                    auto const bytes_read = operation_->connection->read((char*)operation_->buffer.data(), (int)operation_->buffer.size());
                    // Note: treating zero bytes read as a failure condition here to prevent an infinite loop under
                    // those conditions.
                    if (bytes_read <= 0) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Failed reading data - retrying operation";
                        operation_->connection = nullptr;
                        operation_->total_failures++;
                        return;
                    }

                    // Block hash
                    auto hash = HM::calculateHashBinary(
                            HM::kHashTypeSHA256,
                            (unsigned char const*)operation_->buffer.data(),
                            bytes_read
                    );
                    if (!hash.has_value()) {
                        CHARGELAB_LOG_MESSAGE(error) << "Failed hashing block";
                        operation_->connection = nullptr;
                        operation_->total_failures++;
                        return;
                    }

                    operation_->block_hashes.push_back(std::move(hash.value()));
                    operation_->signature_hash->update((unsigned char const*)operation_->buffer.data(), bytes_read);
                    operation_->total_bytes_read += bytes_read;
                    CHARGELAB_LOG_MESSAGE(debug) << "Hashing progress: " << operation_->total_bytes_read << " / " << operation_->content_length;
                }

                if (operation_->total_bytes_read >= operation_->content_length) {
                    checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kDownloaded);

                    auto signature_hash = operation_->signature_hash->finishBinary();
                    if (!signature_hash.has_value()) {
                        checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kInvalidSignature);

                        // L01.FR.03
                        pending_messages_->sendRequest2_0(
                                ocpp2_0::SecurityEventNotificationRequest {
                                        "InvalidFirmwareSignature",
                                        platform_->systemClockNow()
                                },
                                buildPendingMessagePolicy(PendingMessageType::kSecurityEvent)
                        );

                        operation_ = std::nullopt;
                        return;
                    }

                    auto const& firmware = operation_->request.firmware;
                    if (firmware.signingCertificate.has_value() && firmware.signature.has_value()) {
                        auto signature_binary = HM::decodeBase64(firmware.signature->value());
                        if (!signature_binary.has_value()) {
                            checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kInvalidSignature);

                            // L01.FR.03
                            pending_messages_->sendRequest2_0(
                                    ocpp2_0::SecurityEventNotificationRequest {
                                            "InvalidFirmwareSignature",
                                            platform_->systemClockNow()
                                    },
                                    buildPendingMessagePolicy(PendingMessageType::kSecurityEvent)
                            );

                            operation_ = std::nullopt;
                            return;
                        }

                        SignatureAndHash signature_and_hash {};
                        signature_and_hash.sig = signature_binary->data();
                        signature_and_hash.sig_len = signature_binary->size();
                        signature_and_hash.hash = signature_hash->data();
                        signature_and_hash.hash_len = signature_hash->size();

                        auto const valid = platform_->verifyManufacturerCertificate(
                                firmware.signingCertificate->value(),
                                signature_and_hash
                        );
                        if (!valid) {
                            checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kInvalidSignature);

                            // L01.FR.03
                            pending_messages_->sendRequest2_0(
                                    ocpp2_0::SecurityEventNotificationRequest {
                                            "InvalidFirmwareSignature",
                                            platform_->systemClockNow()
                                    },
                                    buildPendingMessagePolicy(PendingMessageType::kSecurityEvent)
                            );

                            operation_ = std::nullopt;
                            return;
                        }

                        checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kSignatureVerified);
                    }

                    std::size_t total_block_hash_size = 0;
                    for (auto const& x : operation_->block_hashes)
                        total_block_hash_size += x.size();
                    CHARGELAB_LOG_MESSAGE(debug) << "Total block hash size (" << operation_->block_hashes.size() << " blocks): " << total_block_hash_size;

                    operation_->expected_signature_hash = std::move(signature_hash);
                    operation_->finished_signature_check = true;
                    operation_->connection = nullptr;
                }

                return;
            }

            if (!operation_->finished_flashing_firmware) {
                // L01.FR.16
                if (operation_->request.firmware.installDateTime.has_value()) {
                    if (operation_->request.firmware.installDateTime->isAfter(platform_->systemClockNow(), false)) {
                        checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kInstallScheduled);
                        return;
                    }
                }

                if ((int)operation_->total_failures > max_retries) {
                    checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kInstallationFailed);
                    operation_ = std::nullopt;
                    return;
                }

                checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kInstalling);
                if (!checkOrRetryConnection(platform_, operation_.value()))
                    return;

                auto const remaining = (int)operation_->content_length - (int)operation_->total_bytes_read;
                if (remaining > 0) {
                    operation_->buffer.resize(kChunkSize);
                    auto const bytes_read = operation_->connection->read((char*)operation_->buffer.data(), (int)operation_->buffer.size());
                    // Note: treating zero bytes read as a failure condition here to prevent an infinite loop under
                    // those conditions.
                    if (bytes_read <= 0) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Failed reading data - retrying operation";
                        operation_->connection = nullptr;
                        operation_->total_failures++;
                        return;
                    }

                    if (operation_->total_bytes_read == 0) {
                        if (operation_->running_firmware_update) {
                            station_->finishUpdateProcess(false);
                            operation_->running_firmware_update = false;
                        }

                        if (station_->startUpdateProcess(operation_->content_length) != StationInterface::Result::kSucceeded) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Failed starting firmware update process";
                            operation_->connection = nullptr;
                            operation_->total_failures++;
                            return;
                        }

                        operation_->running_firmware_update = true;
                    } else {
                        if (!operation_->running_firmware_update) {
                            CHARGELAB_LOG_MESSAGE(error)
                                << "Unexpected state - expected running firmware update operation";
                            operation_->connection = nullptr;
                            operation_->total_failures++;
                            return;
                        }
                    }

                    if (station_->processFirmwareChunk(operation_->buffer.data(), bytes_read) != StationInterface::Result::kSucceeded) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Failed processing firmware chunk";
                        operation_->connection = nullptr;
                        operation_->total_failures++;
                        return;
                    }

                    operation_->signature_hash->update((unsigned char const*)operation_->buffer.data(), bytes_read);
                    operation_->total_bytes_read += bytes_read;
                    CHARGELAB_LOG_MESSAGE(debug) << "Flashing progress: " << operation_->total_bytes_read << " / " << operation_->content_length;
                }

                if (operation_->total_bytes_read >= operation_->content_length) {
                    // TODO: Check signature

                    auto const slot_id = station_->getUpdateSlotId();
                    if (station_->finishUpdateProcess(true) != StationInterface::Result::kSucceeded) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Failed finishing update process";
                        operation_->connection = nullptr;
                        operation_->total_failures++;
                        return;
                    }

                    operation_->finished_flashing_firmware = true;
                    operation_->connection = nullptr;
                    // settings_->ExpectedUpdateFirmwareSlotId.setValueFromString(std::to_string(operation_->request.requestId) + ":" + slot_id);
                    settings_->ExpectedUpdateFirmwareSlotId.setValueFromString(buildExpectedUpdateFirmwareSlotId(
                            operation_->request.requestId, slot_id));
                    checkAndUpdateStatus(ocpp2_0::FirmwareStatusEnumType::kInstallRebooting);

                    reset_->resetOnIdle(ocpp2_0::BootReasonEnumType::kFirmwareUpdate);
                }

                return;
            }
        }

        void performBootChecks1_6() {
            if (performed_boot_checks_)
                return;

            auto const active = station_->getActiveSlotId();
            settings_->ActiveFirmwareSlotId.setValue(active);

            auto const update = settings_->ExpectedUpdateFirmwareSlotId.getValue();
            if (!update.empty()) {
                int request_id;
                std::string slot_id;

                parseExpectedUpdateFirmwareSlotId(update, request_id, slot_id);

                pending_messages_->sendRequest1_6(
                        ocpp1_6::FirmwareStatusNotificationReq {
                                slot_id == active ?
                                ocpp1_6::FirmwareStatus::kInstalled :
                                ocpp1_6::FirmwareStatus::kInstallationFailed
                        },
                        buildPendingMessagePolicy(PendingMessageType::kNotificationEvent)
                );

                settings_->ExpectedUpdateFirmwareSlotId.setValue("");
            }

            performed_boot_checks_ = true;
        }

        void performFirmwareUpdate1_6() {
            if (!operation_.has_value())
                return;

            auto const max_retries = optional::GetOrDefault(
                    operation_->request.retries,
                    settings_->FirmwareUpdateDefaultRetries.getValue()
            );

            if ((int)operation_->total_failures > max_retries) {
                if (operation_->content_length == 0 || operation_->content_length > operation_->total_bytes_read) {
                    checkAndUpdateStatus(ocpp1_6::FirmwareStatus::kDownloadFailed);
                } else {
                    checkAndUpdateStatus(ocpp1_6::FirmwareStatus::kInstallationFailed);
                }
                operation_ = std::nullopt;
                restoreConnector0OperativeIfNeeded();
                return;
            }

            if (!operation_->last_status1_6.has_value()) {
                checkAndUpdateStatus(ocpp1_6::FirmwareStatus::kDownloading);
            } else {
                if (operation_->last_status1_6 == ocpp1_6::FirmwareStatus::kDownloaded) {
                    checkAndUpdateStatus(ocpp1_6::FirmwareStatus::kInstalling);
                }
            }

            if (!checkOrRetryConnection(platform_, operation_.value()))
                return;

            auto const remaining = (int)operation_->content_length - (int)operation_->total_bytes_read;
            if (remaining > 0) {
                operation_->buffer.resize(kChunkSize);
                auto const bytes_read = operation_->connection->read((char*)operation_->buffer.data(), (int)operation_->buffer.size());
                // Note: treating zero bytes read as a failure condition here to prevent an infinite loop under
                // those conditions.
                if (bytes_read <= 0) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed reading data - retrying operation";
                    operation_->connection = nullptr;
                    operation_->total_failures++;
                    return;
                }

                if (operation_->total_bytes_read == 0) {
                    if (operation_->running_firmware_update) {
                        station_->finishUpdateProcess(false);
                        operation_->running_firmware_update = false;
                    }

                    if (station_->startUpdateProcess(operation_->content_length) != StationInterface::Result::kSucceeded) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Failed starting firmware update process";
                        operation_->connection = nullptr;
                        operation_->total_failures++;
                        return;
                    }

                    operation_->running_firmware_update = true;
                } else {
                    if (!operation_->running_firmware_update) {
                        CHARGELAB_LOG_MESSAGE(error)
                            << "Unexpected state - expected running firmware update operation";
                        operation_->connection = nullptr;
                        operation_->total_failures++;
                        return;
                    }
                }

                if (station_->processFirmwareChunk(operation_->buffer.data(), bytes_read) != StationInterface::Result::kSucceeded) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed processing firmware chunk";
                    operation_->connection = nullptr;
                    operation_->total_failures++;
                    return;
                }

                // TODO: For ocpp1.6, the existing approach is to pre-set the hash value to the setting parameter and
                //  read the firmware hash value from the firmware itself. Then compare them to decide if the downloaded firmware is the right one.
                //  What approach is expected here?
                operation_->signature_hash->update((unsigned char const*)operation_->buffer.data(), bytes_read);
                operation_->total_bytes_read += bytes_read;
                CHARGELAB_LOG_MESSAGE(debug) << "Flashing progress: " << operation_->total_bytes_read << " / " << operation_->content_length;
            }

            if (operation_->total_bytes_read >= operation_->content_length) {
                // TODO: Check signature

                if (operation_->last_status1_6 == ocpp1_6::FirmwareStatus::kDownloading) {
                    checkAndUpdateStatus(ocpp1_6::FirmwareStatus::kDownloaded);
                    return;
                } else if (operation_->last_status1_6 == ocpp1_6::FirmwareStatus::kDownloaded) {
                    checkAndUpdateStatus(ocpp1_6::FirmwareStatus::kInstalling);
                    return;
                }

                auto const slot_id = station_->getUpdateSlotId();
                if (station_->finishUpdateProcess(true) != StationInterface::Result::kSucceeded) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed finishing update process";
                    operation_->connection = nullptr;
                    operation_->total_failures++;
                    return;
                }

                //settings_->ExpectedUpdateFirmwareSlotId.setValueFromString(std::to_string(operation_->request.requestId) + ":" + slot_id);
                settings_->ExpectedUpdateFirmwareSlotId.setValueFromString(buildExpectedUpdateFirmwareSlotId(
                        operation_->request.requestId, slot_id));

                operation_ = std::nullopt;
                restoreConnector0OperativeIfNeeded();

                reset_->resetOnIdle(ocpp2_0::BootReasonEnumType::kFirmwareUpdate);
            }
        }

        void restoreConnector0OperativeIfNeeded() {
            if (need_to_restore_connector_0_operative_) {
                connector_status_->setConnector0Inoperative(false);
                need_to_restore_connector_0_operative_ = false;
            }
        }

        static std::string buildExpectedUpdateFirmwareSlotId(int request_id, std::string const& slot_id) {
            return std::to_string(request_id) + ":" + slot_id;
        }

        static void parseExpectedUpdateFirmwareSlotId(std::string const& expectedUpdateFirmwareSlotId,
                                                      int& request_id, std::string& slot_id) {

            auto index = expectedUpdateFirmwareSlotId.find(':');
            if (index != std::string::npos) {
                request_id = optional::GetOrDefault(string::ToInteger(expectedUpdateFirmwareSlotId.substr(0, index)), 0);
                slot_id = expectedUpdateFirmwareSlotId.substr(index+1);
            } else {
                request_id = 0;
                slot_id = expectedUpdateFirmwareSlotId;
            }
        }

        PendingMessagePolicy buildPendingMessagePolicy(PendingMessageType message_type) {
            return PendingMessagePolicy {
                    message_type,
                    kOperationGroupId,
                    settings_->NotificationMessageDefaultRetries.getValue(),
                    settings_->NotificationMessageDefaultRetryInterval.getValue(),
                    kPriorityFirmwareStatusNotification,
                    false,
                    false
            };
        }

    private:
        std::shared_ptr<PlatformInterface> platform_;
        std::shared_ptr<ResetModule> reset_;
        std::shared_ptr<PendingMessagesModule> pending_messages_;
        std::shared_ptr<ConnectorStatusModule> connector_status_; // TODO: Remove dependency on ConnectorStatusModule
        std::shared_ptr<StationInterface> station_;
        std::shared_ptr<Settings> settings_;

        /**
         * Set to true to force an OCPP 2.0.1 or 1.6 firmware status update and false to force an "idle" firmware status
         * update (only applies when an associated trigger message is received with an EVSE filter).
         */
        std::optional<bool> force_update_ = std::nullopt;

        std::optional<detail::FirmwareUpdateOperation<HM>> operation_ = std::nullopt;
        bool performed_boot_checks_ = false;

        bool need_to_restore_connector_0_operative_ { false };
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_FIRMWARE_UPDATE_MODULE_H
