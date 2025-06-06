#ifndef CHARGELAB_OPEN_FIRMWARE_TRANSACTION_MODULE2_0_H
#define CHARGELAB_OPEN_FIRMWARE_TRANSACTION_MODULE2_0_H

#include "openocpp/interface/platform_interface.h"
#include "openocpp/interface/station_interface.h"
#include "openocpp/module/common_templates.h"
#include "openocpp/module/pending_messages_module.h"
#include "openocpp/module/boot_notification_module.h"
#include "openocpp/module/power_management_module2_0.h"
#include "openocpp/module/connector_status_module.h"
#include "openocpp/common/operation_holder.h"
#include "openocpp/common/logging.h"
#include "openocpp/helpers/set.h"
#include "openocpp/protocol/ocpp2_0/types/tx_start_point_values.h"
#include "openocpp/protocol/ocpp2_0/types/tx_stop_point_values.h"

#include <utility>
#include <random>
#include <unordered_set>
#include <deque>
#include <vector>
#include <memory>
#include <string>
#include <optional>

namespace chargelab {
    /**
     * Note: the following are not supported:
     * - F01.FR.03:
     * - 2.6.6.1. TxStartPoint values:
     * -- EnergyTransfer
     * -- DataSigned
     * -- These two options introduce a new type of potentially long-lived "pending transaction" state after the
     *    charging session has been authorised but before either charging starts or the first signed meter values data.
     *
     */

    namespace transaction_module2_0 {
        struct TransactionContainer {
            uint64_t transaction_id;
            SteadyPointMillis start_ts;

            std::optional<ocpp2_0::IdTokenType> id_token;
            std::optional<ocpp2_0::IdTokenType> group_id_token;
            std::string start_transaction_request_id;
            std::optional<ocpp2_0::ChargingProfileType> charging_profile;
            bool authorized;
            bool deauthorized;
            bool plugged_in;

            SystemTimeMillis last_sampled_reading_timestamp;
            SystemTimeMillis last_sampled_ended_meter_values_timestamp;
            SystemTimeMillis last_clock_aligned_reading_timestamp;
            SystemTimeMillis last_clock_aligned_ended_meter_values_timestamp;

            std::optional<ocpp2_0::ChargingStateEnumType> last_charging_status = std::nullopt;
            std::vector<ocpp2_0::MeterValueType> ended_meter_values {};
        };

        struct PendingStartRequest {
            PendingStartRequest(std::shared_ptr<PlatformInterface> const& platform, std::optional<ocpp2_0::EVSEType> evse, ocpp2_0::IdTokenType id_token)
                : created_timestamp {platform->steadyClockNow()},
                  pending_authorize_req {platform},
                  id_token {std::move(id_token)},
                  charging_profile {},
                  remote_start_id {},
                  evse {evse},
                  authorize_finished {false}
            {
            }

            PendingStartRequest(std::shared_ptr<PlatformInterface> const& platform, ocpp2_0::RequestStartTransactionRequest const& req)
                : created_timestamp {platform->steadyClockNow()},
                  pending_authorize_req {platform},
                  id_token {req.idToken},
                  group_id_token {req.groupIdToken},
                  charging_profile {req.chargingProfile},
                  remote_start_id {req.remoteStartId},
                  // F01.FR.01/F01.FR.02
                  authorize_finished {!platform->getSettings()->AuthorizeRemoteTxRequests.getValue()}
            {
                if (req.evseId.has_value()) {
                    evse = ocpp2_0::EVSEType {req.evseId.value()};
                } else {
                    evse = std::nullopt;
                }
            }

            SteadyPointMillis created_timestamp;
            OperationHolder<std::string> pending_authorize_req;
            ocpp2_0::IdTokenType id_token;
            std::optional<ocpp2_0::IdTokenType> group_id_token;
            std::optional<ocpp2_0::ChargingProfileType> charging_profile;
            std::optional<int> remote_start_id;
            std::optional<ocpp2_0::EVSEType> evse;
            std::optional<uint64_t> transaction_id;

            bool authorize_finished;
        };
    }

    class TransactionModule2_0 : public ServiceStateful2_0 {
    private:
        static constexpr int kPriorityStartTransaction = 0;
        static constexpr int kPriorityStopTransaction = 0;
        static constexpr int kPriorityMeterValue = 5;
        static constexpr int kClockDriftThresholdSeconds = 10;
        static constexpr int kMillisecondsInDay = 1000*60*60*24;
        static constexpr std::uint64_t kMeterValuesGroupId = 0x3735DB9FF7144036ull;
        static constexpr int kPriorityMeterValuesNotification = 90;

    public:
        TransactionModule2_0(
                std::shared_ptr<PlatformInterface> platform,
                std::shared_ptr<BootNotificationModule> boot_notification_module,
                std::shared_ptr<PowerManagementModule2_0> power_management_module,
                std::shared_ptr<PendingMessagesModule> pending_messages_module,
                std::shared_ptr<ConnectorStatusModule> connector_status_module,
                std::shared_ptr<StationInterface> station
        ) : platform_(std::move(platform)),
            boot_notification_module_(std::move(boot_notification_module)),
            power_management_module_(std::move(power_management_module)),
            pending_messages_module_(std::move(pending_messages_module)),
            connector_status_module_(std::move(connector_status_module)),
            station_(std::move(station)),
            random_engine_ {std::random_device{}()}
        {
            settings_ = platform_->getSettings();

            std::uniform_int_distribution<int64_t> distribution(
                    std::numeric_limits<int64_t>::min(),
                    std::numeric_limits<int64_t>::max()
            );
            unique_index_ = distribution(random_engine_);

            stop_transaction_supplier_ = std::make_shared<PendingMessagesModule::saved_message_supplier> (
                    [&](std::function<void(detail::PendingMessageWrapper const&)> const& processor) {
                        for (auto& entry : active_transactions_) {
                            if (!entry.second.has_value())
                                continue;

                            processor(generateStopRequest(
                                    entry.second.value(),
                                    entry.first,
                                    ocpp2_0::TriggerReasonEnumType::kAbnormalCondition,
                                    ocpp2_0::ReasonEnumType::kPowerLoss,
                                    std::nullopt
                            ));
                        }
                    }
            );
            pending_messages_module_->registerOnSaveMessageSupplier(stop_transaction_supplier_);
        }

        ~TransactionModule2_0() override {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting TransactionModule2_0";
            if (stop_transaction_supplier_ != nullptr) {
                pending_messages_module_->unregisterOnSaveMessageSupplier(stop_transaction_supplier_);
            }
        }

    public:
        void runStep(ocpp2_0::OcppRemote& remote) override {
            auto const& metadata = station_->getConnectorMetadata();

            // Process an RFID tap if there was a state change
            auto id_token = station_->readToken2_0();
            if (id_token != last_rfid_tag_id_) {
                last_rfid_tag_id_ = id_token;
                if (id_token.has_value())
                    processRfidTap(id_token.value());
            }

            // If an immediate reset was requested stop all active transactions
            if (connector_status_module_->getResetReasonHard()) {
                for (auto& x : active_transactions_) {
                    if (!x.second.has_value())
                        continue;

                    CHARGELAB_LOG_MESSAGE(info) << "Stopping transaction (pending reset) - transaction ID: " << x.second->transaction_id;
                    stopTransaction(
                            x.first,
                            ocpp2_0::TriggerReasonEnumType::kResetCommand,
                            ocpp2_0::ReasonEnumType::kImmediateReset,
                            std::nullopt
                    );
                }
            }

            // Advance plugged in states to track which transactions have ever been connected to a vehicle
            for (auto& entry : active_transactions_) {
                if (!entry.second.has_value())
                    continue;
                if (!entry.first.has_value())
                    continue;

                auto const status = station_->pollConnectorStatus(entry.first.value());
                if (status.has_value() && status->vehicle_connected)
                    entry.second->plugged_in = true;
            }

            // Process plugged in state changes (PlugAndCharge and stopping transaction on disconnect)
            for (auto const& entry : metadata) {
                // Only take action if the connector state has changed. If, for example, a pending RemoteStartTransaction
                // request timed out do not attempt an autostart transaction until the cable is unplugged and plugged
                // back in.
                auto const status = station_->pollConnectorStatus(entry.first);
                if (!status.has_value())
                    continue;
                if (status->vehicle_connected == last_plugged_in_state_[entry.first])
                    continue;

                last_plugged_in_state_[entry.first] = status->vehicle_connected;
                processVehicleConnectedStateChanged(entry.first, status->vehicle_connected);
            }

            // First process any pending start requests for a specific connector
            for (auto& entry : pending_start_req_) {
                if (entry.first.has_value() && entry.first->connectorId.has_value())
                    processPendingStartRequest(remote, entry.second);
            }

            // Then process any pending start requests for a specific EVSE ID
            for (auto& entry : pending_start_req_) {
                if (entry.first.has_value() && !entry.first->connectorId.has_value())
                    processPendingStartRequest(remote, entry.second);
            }

            // Finally, process any pending start requests for the station
            for (auto& entry : pending_start_req_) {
                if (!entry.first.has_value())
                    processPendingStartRequest(remote, entry.second);
            }

            // Process transactions
            for (auto& entry : active_transactions_) {
                if (!entry.second.has_value())
                    continue;

                // E03.FR.05 - TODO
                // Transactions that have not been plugged in time out according to EVConnectionTimeOut
                if (!entry.first.has_value() || !entry.second->plugged_in) {
                    auto const elapsed = platform_->steadyClockNow() - entry.second->start_ts;
                    if (elapsed > settings_->ConnectionTimeOut.getValue()*1000) {
                        CHARGELAB_LOG_MESSAGE(info) << "Stopping transaction (EVConnectionTimeOut) - transaction ID: " << entry.second->transaction_id;
                        stopTransaction(
                                entry.first,
                                ocpp2_0::TriggerReasonEnumType::kEVConnectTimeout,
                                ocpp2_0::ReasonEnumType::kTimeout,
                                std::nullopt
                        );
                        continue;
                    }
                }

                if (shouldTakeReading(entry.second->last_sampled_reading_timestamp, settings_->MeterValueSampleInterval.getValue())) {
                    addSampledReading(
                            entry.second->last_sampled_reading_timestamp,
                            entry.first,
                            entry.second.value(),
                            ocpp2_0::TriggerReasonEnumType::kMeterValuePeriodic,
                            ocpp2_0::ReadingContextEnumType::kSample_Periodic,
                            settings_->MeterValuesSampledData.getValue()
                    );
                }

                if (shouldTakeReading(entry.second->last_sampled_ended_meter_values_timestamp, settings_->SampledDataTxEndedInterval.getValue())) {
                    addEndedReading(
                            entry.second->last_sampled_ended_meter_values_timestamp,
                            entry.first,
                            entry.second.value(),
                            ocpp2_0::ReadingContextEnumType::kSample_Periodic,
                            settings_->StopTxnSampledData.getValue()
                    );
                }

                if (shouldTakeReading(entry.second->last_clock_aligned_reading_timestamp, settings_->ClockAlignedDataInterval.getValue())) {
                    addSampledReading(
                            entry.second->last_clock_aligned_reading_timestamp,
                            entry.first,
                            entry.second.value(),
                            ocpp2_0::TriggerReasonEnumType::kMeterValueClock,
                            ocpp2_0::ReadingContextEnumType::kSample_Clock,
                            settings_->MeterValuesAlignedData.getValue()
                    );
                }

                if (shouldTakeReading(entry.second->last_clock_aligned_ended_meter_values_timestamp, settings_->AlignedDataTxEndedInterval.getValue())) {
                    addEndedReading(
                            entry.second->last_clock_aligned_ended_meter_values_timestamp,
                            entry.first,
                            entry.second.value(),
                            ocpp2_0::ReadingContextEnumType::kSample_Clock,
                            settings_->AlignedDataTxEndedMeasurands.getValue()
                    );
                }

                auto const status = getChargingState(entry.first, entry.second.value());
                if (entry.second->last_charging_status != std::make_optional(status)) {
                    updateTransaction(
                            entry.first,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::TriggerReasonEnumType::kChargingStateChanged,
                            platform_->systemClockNow()
                    );
                }
            }

            // Process non-transaction meter values
            auto const last_trigger_delta = platform_->systemClockNow() - last_non_transaction_meter_value_trigger_;
            if (shouldTakeReading(last_non_transaction_meter_value_trigger_, settings_->ClockAlignedDataInterval.getValue())) {
                if (std::abs(last_trigger_delta) < kMillisecondsInDay) {
                    for (auto const& entry : station_->getConnectorMetadata()) {
                        auto const& transaction = active_transactions_[entry.first];
                        if (transaction.has_value())
                            continue;

                        auto meter_values = getMeterValues(
                                entry.first,
                                ocpp2_0::ReadingContextEnumType::kSample_Clock,
                                settings_->MeterValuesAlignedData.getValue(),
                                last_non_transaction_meter_value_trigger_
                        );
                        if (!meter_values.has_value())
                            continue;

                        pending_messages_module_->sendRequest2_0(
                                ocpp2_0::MeterValuesRequest {
                                        entry.first.id,
                                        std::move(meter_values.value())
                                },
                                PendingMessagePolicy {
                                        PendingMessageType::kNotificationEvent,
                                        kMeterValuesGroupId,
                                        settings_->TransactionMessageAttempts.getValue(),
                                        settings_->TransactionMessageRetryInterval.getValue(),
                                        kPriorityMeterValuesNotification,
                                        false,
                                        true
                                }
                        );
                    }
                }
            }

            // Update power management state and stop/start any active transactions
            for (auto& entry : active_transactions_) {
                if (!entry.first.has_value() || !entry.second.has_value())
                    continue;
                if (!entry.first->connectorId.has_value())
                    continue;

                auto const status = station_->pollConnectorStatus(entry.first.value());
                if (!status.has_value()) {
                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected state: connector did not exist for running transaction - dropping: " << entry.second->transaction_id;
                    entry.second = std::nullopt;
                    continue;
                }

                if (entry.second->authorized && !entry.second->deauthorized && status->vehicle_connected) {
                    power_management_module_->onActiveTransactionStarted(
                            entry.first->id,
                            std::move(entry.second->charging_profile),
                            entry.second->start_ts
                    );
                    station_->setChargingEnabled(entry.first.value(), true);
                } else {
                    station_->setChargingEnabled(entry.first.value(), false);
                }
            }
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::RequestStartTransactionResponse>> onRequestStartTransactionReq(
                const ocpp2_0::RequestStartTransactionRequest &req
        ) override {
            auto const& metadata = station_->getConnectorMetadata();
            if (req.evseId.has_value()) {
                bool found = false;
                for (auto const& x : metadata) {
                    if (x.first.id == req.evseId.value()) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    // Not specified in F01 or F02; adopting a Rejected response. Could arguably be an error response
                    // as well, but does not seem to be specified in the standard.
                    CHARGELAB_LOG_MESSAGE(info) << "Rejected RequestStartTransactionRequest - evseId not found";
                    return ocpp2_0::RequestStartTransactionResponse{ocpp2_0::RequestStartStopStatusEnumType::kRejected};
                }
            }

            if (req.chargingProfile.has_value()) {
                // F01.FR.09
                if (req.chargingProfile->chargingProfilePurpose != ocpp2_0::ChargingProfilePurposeEnumType::kTxProfile) {
                    // F01.FR.26
                    CHARGELAB_LOG_MESSAGE(info) << "Rejected RequestStartTransactionRequest - bad charging profile purpose";
                    return ocpp2_0::RequestStartTransactionResponse{ocpp2_0::RequestStartStopStatusEnumType::kRejected};
                }

                // F01.FR.11
                if (req.chargingProfile->transactionId.has_value()) {
                    // F01.FR.26
                    CHARGELAB_LOG_MESSAGE(info) << "Rejected RequestStartTransactionRequest - unexpected charging profile transaction ID";
                    return ocpp2_0::RequestStartTransactionResponse{ocpp2_0::RequestStartStopStatusEnumType::kRejected};
                }
            }

            // F02.FR.25
            // TODO: Need to pull in information from other modules or the device model?

            // F02.FR.26
            // Note: need to determine which EVSE IDs are available. The convention here is: multiple connectors may be
            // present under a single EVSE ID, but only one can be used at a time.
            // TODO: Is there something in the standard I can refer to here?
            std::set<int> available_evse_ids;
            for (auto const& x : metadata)
                available_evse_ids.insert(x.first.id);
            for (auto const& x : active_transactions_) {
                // Note: skipping "de-authorized" transactions to allow a PendingStartRequest under those conditions to
                // re-authorize the transaction (F02.FR.26). This request may still need to be authorized based on the
                // AuthorizeRemoteTxRequests setting.
                if (x.second.has_value() && x.second->authorized && x.first.has_value())
                    available_evse_ids.erase(x.first->id);
            }

            bool available_connector;
            if (req.evseId.has_value()) {
                available_connector = available_evse_ids.find(req.evseId.value()) != available_evse_ids.end();
            } else {
                available_connector = !available_evse_ids.empty();
            }

            if (!available_connector) {
                CHARGELAB_LOG_MESSAGE(info) << "Rejected RequestStartTransactionRequest - no EVSE available";
                return ocpp2_0::RequestStartTransactionResponse{ocpp2_0::RequestStartStopStatusEnumType::kRejected};
            }

            // Note: adopting the convention that a new RequestStartTransactionRequest request will replace an existing
            // pending one, rather than be blocked by it. This doesn't appear to be clear from the specification;
            // F02.FR.26 specifies what should happen here if there is already an *authorized transaction*, but does not
            // specify a behaviour when there's a pending request that has not started a transaction yet.
            std::optional<ocpp2_0::EVSEType> evse {};
            if (req.evseId.has_value())
                evse->id = req.evseId.value();

            pending_start_req_[evse] = transaction_module2_0::PendingStartRequest {platform_, req};
            return ocpp2_0::RequestStartTransactionResponse{ocpp2_0::RequestStartStopStatusEnumType::kAccepted};
        }

        void onAuthorizeRsp(
                const std::string &unique_id,
                const std::variant<ocpp2_0::AuthorizeResponse, ocpp2_0::CallError> &rsp
        ) override {
            bool found = false;
            for (auto& entry : pending_start_req_) {
                if (!entry.second.has_value())
                    continue;
                if (entry.second->pending_authorize_req != unique_id)
                    continue;

                found = true;
                if (std::holds_alternative<ocpp2_0::AuthorizeResponse>(rsp)) {
                    // Note: only clearing the operation if a valid response was received, otherwise treating as absent
                    // and retrying after configured timeout.
                    entry.second->pending_authorize_req = kNoOperation;

                    auto const& value = std::get<ocpp2_0::AuthorizeResponse>(rsp);
                    switch (value.idTokenInfo.status) {
                        case ocpp2_0::AuthorizationStatusEnumType::kAccepted:
                            if (stopByGroupId(entry.second->id_token, value.idTokenInfo.groupIdToken)) {
                                // C09.FR.03
                                CHARGELAB_LOG_MESSAGE(info) << "Interpreting Authorize response as a stop transaction request based on group ID";
                                entry.second = std::nullopt;
                            } else {
                                CHARGELAB_LOG_MESSAGE(info) << "Matched Authorize response and set authorize finished to true";
                                entry.second->group_id_token = value.idTokenInfo.groupIdToken;
                                entry.second->authorize_finished = true;
                            }
                            break;

                        case ocpp2_0::AuthorizationStatusEnumType::kValueNotFoundInEnum:
                        case ocpp2_0::AuthorizationStatusEnumType::kBlocked:
                        case ocpp2_0::AuthorizationStatusEnumType::kConcurrentTx:
                        case ocpp2_0::AuthorizationStatusEnumType::kExpired:
                        case ocpp2_0::AuthorizationStatusEnumType::kInvalid:
                        case ocpp2_0::AuthorizationStatusEnumType::kNoCredit:
                        case ocpp2_0::AuthorizationStatusEnumType::kNotAllowedTypeEVSE:
                        case ocpp2_0::AuthorizationStatusEnumType::kNotAtThisLocation:
                        case ocpp2_0::AuthorizationStatusEnumType::kNotAtThisTime:
                        case ocpp2_0::AuthorizationStatusEnumType::kUnknown:
                            // F01.FR.03
                            CHARGELAB_LOG_MESSAGE(info) << "Matched Authorize response and removed pending start request";
                            entry.second = std::nullopt;
                            break;
                    }
                } else {
                    CHARGELAB_LOG_MESSAGE(warning) << "Error response to Authorize request: " << std::get<ocpp2_0::CallError> (rsp);
                }
            }

            if (!found) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed matching Authorize response against active Authorize requests";
            }
        }

        void onTransactionEventRsp(
                const std::string &unique_id,
                const std::variant<ocpp2_0::TransactionEventResponse, ocpp2_0::CallError> &rsp
        ) override {
            // Note: this is processed here in addition to pending_messages to allow this module to take action if the
            // back-end does not accept the provided idTag.
            for (auto& entry : active_transactions_) {
                if (!entry.second.has_value())
                    continue;
                if (unique_id != entry.second->start_transaction_request_id)
                    continue;

                if (std::holds_alternative<ocpp2_0::TransactionEventResponse>(rsp)) {
                    auto const &value = std::get<ocpp2_0::TransactionEventResponse>(rsp);
                    if (value.idTokenInfo.has_value()) {
                        switch (value.idTokenInfo->status) {
                            case ocpp2_0::AuthorizationStatusEnumType::kAccepted:
                                break;

                            case ocpp2_0::AuthorizationStatusEnumType::kValueNotFoundInEnum:
                            case ocpp2_0::AuthorizationStatusEnumType::kBlocked:
                            case ocpp2_0::AuthorizationStatusEnumType::kConcurrentTx:
                            case ocpp2_0::AuthorizationStatusEnumType::kExpired:
                            case ocpp2_0::AuthorizationStatusEnumType::kInvalid:
                            case ocpp2_0::AuthorizationStatusEnumType::kNoCredit:
                            case ocpp2_0::AuthorizationStatusEnumType::kNotAllowedTypeEVSE:
                            case ocpp2_0::AuthorizationStatusEnumType::kNotAtThisLocation:
                            case ocpp2_0::AuthorizationStatusEnumType::kNotAtThisTime:
                            case ocpp2_0::AuthorizationStatusEnumType::kUnknown:
                                // TODO: Support MaxEnergyOnInvalidId?
                                if (settings_->StopTransactionOnInvalidId.getValue()) {
                                    if (set::containsAny(
                                            getStopPoints(),
                                            ocpp2_0::TxStopPointValues::kAuthorized,
                                            ocpp2_0::TxStopPointValues::kPowerPathClosed,
                                            ocpp2_0::TxStopPointValues::kEnergyTransfer
                                    )) {
                                        // C15.FR.04
                                        CHARGELAB_LOG_MESSAGE(info) << "Stopping transaction (StopTransactionOnInvalidId) - transaction ID: " << entry.second->transaction_id;
                                        stopTransaction(
                                                entry.first,
                                                ocpp2_0::TriggerReasonEnumType::kDeauthorized,
                                                ocpp2_0::ReasonEnumType::kDeAuthorized,
                                                std::nullopt
                                        );
                                    } else {
                                        // C15.FR.03
                                        deauthorizeTransaction(
                                                entry.first,
                                                entry.second.value(),
                                                ocpp2_0::TriggerReasonEnumType::kDeauthorized
                                        );
                                    }
                                } else {
                                    if (set::containsAny(getStopPoints(), ocpp2_0::TxStopPointValues::kEnergyTransfer)) {
                                        // TODO: it's not clear what trigger reasons should be used here
                                        CHARGELAB_LOG_MESSAGE(info) << "Stopping transaction (EnergyTransfer) - transaction ID: " << entry.second->transaction_id;
                                        stopTransaction(
                                                entry.first,
                                                ocpp2_0::TriggerReasonEnumType::kDeauthorized,
                                                ocpp2_0::ReasonEnumType::kDeAuthorized,
                                                std::nullopt
                                        );
                                    } else {
                                        // C15.FR.03
                                        deauthorizeTransaction(
                                                entry.first,
                                                entry.second.value(),
                                                ocpp2_0::TriggerReasonEnumType::kChargingStateChanged
                                        );
                                    }
                                }
                                break;
                        }
                    }
                } else {
                    CHARGELAB_LOG_MESSAGE(warning) << "Error response to StartTransaction request: " << std::get<ocpp2_0::CallError>(rsp);
                }
            }
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::RequestStopTransactionResponse>> onRequestStopTransactionReq(
                const ocpp2_0::RequestStopTransactionRequest &req
        ) override {
            for (auto& entry : active_transactions_) {
                if (!entry.second.has_value())
                    continue;
                if (!string::EqualsIgnoreCaseAscii(std::to_string(entry.second->transaction_id), req.transactionId.value()))
                    continue;

                CHARGELAB_LOG_MESSAGE(info) << "Stopping transaction (RequestStopTransactionReq) - transaction ID: " << entry.second->transaction_id;
                stopTransaction(
                        entry.first,
                        ocpp2_0::TriggerReasonEnumType::kRemoteStop,
                        ocpp2_0::ReasonEnumType::kRemote,
                        std::nullopt
                );
                return ocpp2_0::RequestStopTransactionResponse {ocpp2_0::RequestStartStopStatusEnumType::kAccepted};
            }

            if (settings_->StopTransactionWithDifferentId.getValue()) {
                return ocpp2_0::RequestStopTransactionResponse{ocpp2_0::RequestStartStopStatusEnumType::kAccepted};
            }

            return ocpp2_0::RequestStopTransactionResponse {ocpp2_0::RequestStartStopStatusEnumType::kRejected};
        }

        std::optional<ocpp2_0::ResponseToRequest <ocpp2_0::TriggerMessageResponse>>
        onTriggerMessageReq(const ocpp2_0::TriggerMessageRequest &req) override {
            if (req.requestedMessage == ocpp2_0::MessageTriggerEnumType::kMeterValues) {
                // TODO: Not implemented; MeterValue messages are not used at the moment.
                return ocpp2_0::TriggerMessageResponse {ocpp2_0::TriggerMessageStatusEnumType::kRejected};
            } else if (req.requestedMessage == ocpp2_0::MessageTriggerEnumType::kTransactionEvent) {
                for (auto& entry : active_transactions_) {
                    if (!entry.first.has_value() || !entry.second.has_value())
                        continue;

                    // F06.FR.11
                    if (req.evse.has_value()) {
                        if (req.evse->id != entry.first->id)
                            continue;
                        if (req.evse->connectorId.has_value() && req.evse->connectorId != entry.first->connectorId)
                            continue;
                    }

                    addSampledReading(
                            platform_->systemClockNow(),
                            entry.first,
                            entry.second.value(),
                            ocpp2_0::TriggerReasonEnumType::kTrigger,
                            ocpp2_0::ReadingContextEnumType::kTrigger,
                            // F06.FR.07
                            settings_->MeterValuesSampledData.getValue()
                    );

                    return ocpp2_0::TriggerMessageResponse {ocpp2_0::TriggerMessageStatusEnumType::kAccepted};
                }

                CHARGELAB_LOG_MESSAGE(error) << "Transaction not found";
                return ocpp2_0::TriggerMessageResponse {ocpp2_0::TriggerMessageStatusEnumType::kRejected};
            } else {
                return std::nullopt;
            }
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetTransactionStatusResponse>>
        onGetTransactionStatusReq(const ocpp2_0::GetTransactionStatusRequest &request) override {
            ocpp2_0::GetTransactionStatusResponse response;
            // E14.FR.06 - ongoing indicator is only set if transactionId is present
            if (request.transactionId.has_value()) {
                response.ongoingIndicator = false;

                for (auto const &x: active_transactions_) {
                    if (!x.second.has_value())
                        continue;

                    auto const id = ocpp2_0::IdentifierStringPrimitive<36>{std::to_string(x.second->transaction_id)};
                    if (request.transactionId.value() != id)
                        continue;

                    response.ongoingIndicator = true;
                }
            }

            pending_messages_module_->visitPending([&](PendingMessagePolicy const& policy, std::string const&) {
                if (policy.message_type != PendingMessageType::kTransactionEvent)
                    return;

                if (request.transactionId.has_value()) {
                    if (!policy.group_id.has_value())
                        return;

                    auto const id = ocpp2_0::IdentifierStringPrimitive<36> {std::to_string(policy.group_id.value())};
                    if (request.transactionId.value() != id)
                        return;
                }

                response.messagesInQueue = true;
            });

            return response;
        }

    private:
        transaction_module2_0::TransactionContainer& startTransaction(
                std::optional<ocpp2_0::EVSEType> evse,
                std::optional<ocpp2_0::IdTokenType> const& id_token,
                std::optional<ocpp2_0::IdTokenType> const& group_id_token,
                ocpp2_0::TriggerReasonEnumType const& trigger_reason,
                bool authorized,
                SystemTimeMillis timestamp,
                std::optional<std::vector<ocpp2_0::MeterValueType>> meter_values = std::nullopt,
                std::optional<ocpp2_0::ChargingProfileType> const& charging_profile = std::nullopt,
                std::optional<int> const& remote_start_id = std::nullopt
        ) {
            // Note: treating a timestamp prefix and a random suffix as sufficient to satisfy requirements in
            // "1.2. TransactionId generation." Technically does not guarantee transaction ID uniqueness, however:
            // - Recommended UUIDs may not guarantee uniqueness when the system clock may be inaccurate
            // - Requirement is not strict; "[...] it *should* never use the TransactionId twice" (emphasis added).
            // -- SHOULD requirements are documented in RFC2119
            auto const clock_fragment = static_cast<uint64_t>(static_cast<uint32_t>(platform_->systemClockNow()));
            auto const random_fragment = static_cast<uint64_t>(static_cast<uint32_t>(unique_index_++));
            auto const transaction_id = (clock_fragment << sizeof(uint32_t)*CHAR_BIT) | random_fragment;

            auto const now = platform_->systemClockNow();
            auto aligned_interval_sample = settings_->ClockAlignedDataInterval.getValue() * 1000;
            if (aligned_interval_sample <= 0)
                aligned_interval_sample = kMillisecondsInDay;

            auto aligned_interval_ended = settings_->AlignedDataTxEndedInterval.getValue() * 1000;
            if (aligned_interval_ended <= 0)
                aligned_interval_ended = kMillisecondsInDay;

            auto& active = active_transactions_[evse] = transaction_module2_0::TransactionContainer {
                transaction_id,
                platform_->steadyClockNow(),
                id_token,
                group_id_token,
                "",
                charging_profile,
                authorized,
                false,
                trigger_reason == ocpp2_0::TriggerReasonEnumType::kCablePluggedIn ? true : false,
                floorToSecond(now),
                floorToSecond(now),
                static_cast<SystemTimeMillis> ((now/aligned_interval_sample)*aligned_interval_sample),
                static_cast<SystemTimeMillis> ((now/aligned_interval_ended)*aligned_interval_ended)
            };

            active->start_transaction_request_id = pending_messages_module_->sendRequest2_0(
                    ocpp2_0::TransactionEventRequest {
                            ocpp2_0::TransactionEventEnumType::kStarted,
                            timestamp,
                            trigger_reason,
                            0,
                            !isWebsocketConnected(),
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::TransactionType {
                                    std::to_string(active->transaction_id),
                                    active->last_charging_status = getChargingState(evse, active.value()),
                                    std::nullopt,
                                    std::nullopt,
                                    remote_start_id
                            },
                            active->id_token,
                            evse,
                            std::move(meter_values)
                    },
                    PendingMessagePolicy {
                            PendingMessageType::kTransactionEvent,
                            transaction_id,
                            settings_->TransactionMessageAttempts.getValue(),
                            settings_->TransactionMessageRetryInterval.getValue(),
                            kPriorityStartTransaction,
                            false,
                            true,
                            true
                    }
            );

            return active.value();
        }

        bool updateTransaction(
                std::optional<ocpp2_0::EVSEType> evse,
                std::optional<ocpp2_0::IdTokenType> const& id_token,
                std::optional<ocpp2_0::IdTokenType> const& group_id_token,
                ocpp2_0::TriggerReasonEnumType const& trigger_reason,
                SystemTimeMillis timestamp,
                std::optional<std::vector<ocpp2_0::MeterValueType>> meter_values = std::nullopt
        ) {
            auto& active = active_transactions_[evse];
            if (!active.has_value())
                return false;

            if (id_token.has_value())
                active->id_token = id_token;
            if (group_id_token.has_value())
                active->group_id_token = group_id_token;

            pending_messages_module_->sendRequest2_0(
                    ocpp2_0::TransactionEventRequest {
                            ocpp2_0::TransactionEventEnumType::kUpdated,
                            timestamp,
                            trigger_reason,
                            0,
                            !isWebsocketConnected(),
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::TransactionType {
                                    std::to_string(active->transaction_id),
                                    active->last_charging_status = getChargingState(evse, active.value())
                            },
                            id_token,
                            evse,
                            std::move(meter_values)
                    },
                    PendingMessagePolicy {
                            PendingMessageType::kTransactionEvent,
                            active->transaction_id,
                            settings_->TransactionMessageAttempts.getValue(),
                            settings_->TransactionMessageRetryInterval.getValue(),
                            kPriorityStartTransaction,
                            false,
                            true
                    }
            );
            return true;
        }

        detail::PendingMessageWrapper generateStopRequest(
                transaction_module2_0::TransactionContainer const& transaction,
                std::optional<ocpp2_0::EVSEType> const& evse,
                ocpp2_0::TriggerReasonEnumType const& trigger_reason,
                ocpp2_0::ReasonEnumType const& stopped_reason,
                std::optional<ocpp2_0::IdTokenType> id_token
        ) {
            std::optional<ocpp2_0::ChargingStateEnumType> last_charging_status = std::nullopt;
            if (evse.has_value()) {
                // TODO: Can this be generalised to a common getChargingState function?
                auto const& status = station_->pollConnectorStatus(evse.value());
                if (status.has_value() && status->vehicle_connected) {
                    last_charging_status = ocpp2_0::ChargingStateEnumType::kEVConnected;
                } else {
                    last_charging_status = ocpp2_0::ChargingStateEnumType::kIdle;
                }
            }

            auto const now = floorToSecond(platform_->systemClockNow());
            std::optional<std::vector<ocpp2_0::MeterValueType>> meter_values = transaction.ended_meter_values;

            // Add the final meter value reading as per the description in 2.7.4:
            // "... every SampledDataTxEndedInterval seconds from the start of the transaction until and including the
            // last measurands at the end of the transaction."
            auto const final_values = getMeterValues(
                    evse,
                    ocpp2_0::ReadingContextEnumType::kTransaction_End,
                    settings_->StopTxnSampledData.getValue(),
                    now
            );
            if (final_values.has_value()) {
                for (auto const &x : final_values.value())
                    meter_values->push_back(x);
            }

            if (meter_values->empty())
                meter_values = std::nullopt;

            return pending_messages_module_->generateRequest2_0(
                    ocpp2_0::TransactionEventRequest {
                            ocpp2_0::TransactionEventEnumType::kEnded,
                            now,
                            trigger_reason,
                            0,
                            !isWebsocketConnected(),
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::TransactionType {
                                    std::to_string(transaction.transaction_id),
                                    last_charging_status,
                                    std::nullopt,
                                    stopped_reason
                            },
                            // TODO: Need to determine if we should be including the original ID token here or not.
                            //  Adopting the convention that this shouldn't be set unless the user taps their RFID card
                            //  for now; *may break currently passing tests*.
                            std::move(id_token),
                            evse,
                            std::move(meter_values)
                    },
                    PendingMessagePolicy {
                            PendingMessageType::kTransactionEvent,
                            transaction.transaction_id,
                            settings_->TransactionMessageAttempts.getValue(),
                            settings_->TransactionMessageRetryInterval.getValue(),
                            kPriorityStopTransaction,
                            false,
                            // Note: set to true here because pending messages *may be dropped* if the device is offline
                            // for an extended period of time, in which case the pending message module should attempt
                            // to correct the sequence numbers (best effort).
                            true,
                            true
                    }
            );
        }

        void stopTransaction(
                std::optional<ocpp2_0::EVSEType> const& evse,
                ocpp2_0::TriggerReasonEnumType const& trigger_reason,
                ocpp2_0::ReasonEnumType const& stopped_reason,
                std::optional<ocpp2_0::IdTokenType> id_token
        ) {
            auto& active = active_transactions_[evse];
            if (!active.has_value())
                return;

            if (evse.has_value()) {
                // E07 - Alternative scenario description
                // Note: this order is required for test case TC_F_08_CS so that the event comes with an EVConnected
                //       charging state.
                station_->setChargingEnabled(evse.value(), false);

                // TODO: Can this be generalised to a common getChargingState function?
                auto const& status = station_->pollConnectorStatus(evse.value());
                if (status.has_value() && status->vehicle_connected) {
                    active->last_charging_status = ocpp2_0::ChargingStateEnumType::kEVConnected;
                } else {
                    active->last_charging_status = ocpp2_0::ChargingStateEnumType::kIdle;
                }

                power_management_module_->onActiveTransactionFinished(evse->id);
            }

            auto const now = floorToSecond(platform_->systemClockNow());
            std::optional<std::vector<ocpp2_0::MeterValueType>> meter_values = std::move(active->ended_meter_values);

            // Add the final meter value reading as per the description in 2.7.4:
            // "... every SampledDataTxEndedInterval seconds from the start of the transaction until and including the
            // last measurands at the end of the transaction."
            auto const final_values = getMeterValues(
                    evse,
                    ocpp2_0::ReadingContextEnumType::kTransaction_End,
                    settings_->StopTxnSampledData.getValue(),
                    now
            );
            if (final_values.has_value()) {
                for (auto const &x : final_values.value())
                    meter_values->push_back(x);
            }

            if (meter_values->empty())
                meter_values = std::nullopt;

            pending_messages_module_->sendRequest2_0(
                    ocpp2_0::TransactionEventRequest {
                            ocpp2_0::TransactionEventEnumType::kEnded,
                            now,
                            trigger_reason,
                            0,
                            !isWebsocketConnected(),
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::TransactionType {
                                std::to_string(active->transaction_id),
                                active->last_charging_status,
                                std::nullopt,
                                stopped_reason
                            },
                            // TODO: Need to determine if we should be including the original ID token here or not.
                            //  Adopting the convention that this shouldn't be set unless the user taps their RFID card
                            //  for now; *may break currently passing tests*.
                            std::move(id_token),
                            evse,
                            std::move(meter_values)
                    },
                    PendingMessagePolicy {
                            PendingMessageType::kTransactionEvent,
                            active->transaction_id,
                            settings_->TransactionMessageAttempts.getValue(),
                            settings_->TransactionMessageRetryInterval.getValue(),
                            kPriorityStopTransaction,
                            false,
                            // Note: set to true here because pending messages *may be dropped* if the device is offline
                            // for an extended period of time, in which case the pending message module should attempt
                            // to correct the sequence numbers (best effort).
                            true,
                            true
                    }
            );
            active = std::nullopt;
        }

        void processRfidTap(ocpp2_0::IdTokenType const& id_token) {
            // Check if a running transaction already uses this tag; if so it should be interpreted as a stop
            // request.
            if (stopByIdTag(id_token))
                return;

            // Otherwise treat as a new pending start request
            pending_start_req_[std::nullopt] = transaction_module2_0::PendingStartRequest{platform_, std::nullopt, id_token};
        }

        bool stopByIdTag(ocpp2_0::IdTokenType id_token) {
            for (auto& x : active_transactions_) {
                if (!x.second.has_value() || !x.second->id_token.has_value())
                    continue;

                // TODO: Assuming validation is based on the token value exclusively, not the other metadata present.
                auto const value = x.second->id_token.value();
                if (value.type == id_token.type && value.idToken == id_token.idToken) {
                    auto const& stop_points = getStopPoints();
                    if (set::containsAny(stop_points, ocpp2_0::TxStopPointValues::kAuthorized, ocpp2_0::TxStopPointValues::kPowerPathClosed)) {
                        CHARGELAB_LOG_MESSAGE(info) << "Stopping transaction (RFID) - transaction ID: " << x.second->transaction_id;
                        stopTransaction(
                                x.first,
                                ocpp2_0::TriggerReasonEnumType::kStopAuthorized,
                                ocpp2_0::ReasonEnumType::kLocal,
                                id_token
                        );
                    } else {
                        deauthorizeTransaction(
                                x.first,
                                x.second.value(),
                                ocpp2_0::TriggerReasonEnumType::kStopAuthorized
                        );
                    }

                    return true;
                }
            }

            return false;
        }

        void deauthorizeTransaction(
                std::optional<ocpp2_0::EVSEType> evse_id,
                transaction_module2_0::TransactionContainer& transaction,
                ocpp2_0::TriggerReasonEnumType trigger_reason
        ) {
            if (evse_id.has_value()) {
                station_->setChargingEnabled(evse_id.value(), false);
            }

            transaction.deauthorized = true;
            updateTransaction(
                    evse_id,
                    std::nullopt,
                    std::nullopt,
                    trigger_reason,
                    platform_->systemClockNow()
            );
        }

        bool stopByGroupId(ocpp2_0::IdTokenType id_token, std::optional<ocpp2_0::IdTokenType> const& group_id) {
            if (!group_id.has_value())
                return false;

            for (auto& x : active_transactions_) {
                if (!x.second.has_value() || !x.second->group_id_token.has_value())
                    continue;

                // TODO: Assuming validation is based on the token value exclusively, not the other metadata present.
                auto const value = x.second->group_id_token.value();
                if (value.type == group_id->type && value.idToken == group_id->idToken) {
                    auto const& stop_points = getStopPoints();
                    if (set::containsAny(stop_points, ocpp2_0::TxStopPointValues::kAuthorized, ocpp2_0::TxStopPointValues::kPowerPathClosed)) {
                        CHARGELAB_LOG_MESSAGE(info) << "Stopping transaction (group ID) - transaction ID: "
                                                    << x.second->transaction_id;
                        stopTransaction(
                                x.first,
                                ocpp2_0::TriggerReasonEnumType::kStopAuthorized,
                                ocpp2_0::ReasonEnumType::kLocal,
                                id_token
                        );
                    } else {
                        deauthorizeTransaction(
                                x.first,
                                x.second.value(),
                                ocpp2_0::TriggerReasonEnumType::kDeauthorized
                        );
                    }

                    return true;
                }
            }

            return false;
        }

        void processVehicleConnectedStateChanged(ocpp2_0::EVSEType const& evse, bool connected) {
            assert(evse.connectorId.has_value());
            CHARGELAB_LOG_MESSAGE(info) << "Connector state changed (" << evse << "): connected=" << connected;
            CHARGELAB_LOG_MESSAGE(info) << "Running transaction on connector: " << active_transactions_[evse].has_value();

            if (active_transactions_[evse].has_value()) {
                if (!connected) {
                    if (set::containsAny(getStopPoints(), ocpp2_0::TxStopPointValues::kEVConnected, ocpp2_0::TxStopPointValues::kPowerPathClosed)) {
                        CHARGELAB_LOG_MESSAGE(info) << "Stopping transaction (EVConnected|PowerPathClosed) - transaction ID: " << active_transactions_[evse]->transaction_id;
                        stopTransaction(
                                evse,
                                ocpp2_0::TriggerReasonEnumType::kEVCommunicationLost,
                                ocpp2_0::ReasonEnumType::kEVDisconnected,
                                std::nullopt
                        );
                    }
                } else {
                    updateTransaction(
                            evse,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::TriggerReasonEnumType::kEVCommunicationLost,
                            platform_->systemClockNow()
                    );
                }

                // Note: PlugAndCharge should only trigger if there's no activate transaction.
                return;
            }

            if (connected) {
                // First attempt to assign a running transaction to the connector
                // Start with transactions which are assigned to an EVSE ID but not a connector
                for (auto& x : active_transactions_) {
                    if (!x.second.has_value())
                        continue;
                    if (!x.first.has_value())
                        continue;
                    if (x.first->connectorId.has_value())
                        continue;

                    auto const now = platform_->systemClockNow();
                    std::swap(active_transactions_[x.first], active_transactions_[evse]);
                    updateTransaction(
                            evse,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::TriggerReasonEnumType::kCablePluggedIn,
                            now,
                            // E01.FR.17
                            getMeterValues(
                                    evse,
                                    ocpp2_0::ReadingContextEnumType::kTransaction_Begin,
                                    settings_->SampledDataTxStartedMeasurands.getValue(),
                                    now
                            )
                    );

                    // Allow PlugAndCharge to trigger a pending start request if not already authorized
                    if (active_transactions_[evse]->authorized)
                        return;
                }

                // Next try to associate with any "open" transaction
                for (auto& x : active_transactions_) {
                    if (!x.second.has_value())
                        continue;
                    if (x.first.has_value())
                        continue;

                    auto const now = platform_->systemClockNow();
                    std::swap(active_transactions_[x.first], active_transactions_[evse]);
                    updateTransaction(
                            evse,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::TriggerReasonEnumType::kCablePluggedIn,
                            now,
                            // E01.FR.17
                            getMeterValues(
                                    evse,
                                    ocpp2_0::ReadingContextEnumType::kTransaction_Begin,
                                    settings_->SampledDataTxStartedMeasurands.getValue(),
                                    now
                            )
                    );

                    // Allow PlugAndCharge to trigger a pending start request if not already authorized
                    if (active_transactions_[evse]->authorized)
                        return;
                }

                // Finally, start a new transaction if possible
                if (!connector_status_module_->getPendingReset()) {
                    auto const &start_points = getStartPoints();
                    if (set::contains(start_points, ocpp2_0::TxStartPointValues::kEVConnected)) {
                        // Start transaction
                        auto const now = floorToSecond(platform_->systemClockNow());
                        auto const& result = startTransaction(
                                evse,
                                std::nullopt,
                                std::nullopt,
                                ocpp2_0::TriggerReasonEnumType::kCablePluggedIn,
                                false,
                                now,
                                // E01.FR.09
                                getMeterValues(
                                        evse,
                                        ocpp2_0::ReadingContextEnumType::kTransaction_Begin,
                                        settings_->SampledDataTxStartedMeasurands.getValue(),
                                        now
                                )
                        );
                        CHARGELAB_LOG_MESSAGE(info) << "Started new transaction (EVConnected) - transaction ID: " << result.transaction_id;

                        // Allow PlugAndCharge to trigger a pending start request if not already authorized
                        // Note: this transaction will never be authorized at this point, but leaving this check in
                        //       place for clarity.
                        if (active_transactions_[evse]->authorized)
                            return;
                    }
                }
            }

            auto plug_and_charge_id = settings_->PlugAndChargeId.getValue();
            if (connected && !plug_and_charge_id.empty()) {
                // If there's another potentially applicable start operation pending (remote start request, RFID tap,
                // etc) it should take precedence and autostart should be ignored.
                bool found = false;
                for (auto const& x : pending_start_req_) {
                    if (!x.first.has_value()) {
                        found = true;
                        break;
                    }
                    if (x.first->id == evse.id) {
                        if (!x.first->connectorId.has_value()) {
                            found = true;
                            break;
                        }
                        if (x.first->connectorId.value() == evse.connectorId.value()) {
                            found = true;
                            break;
                        }
                    }
                }

                if (found)
                    return;

                // Add the PlugAndCharge entry
                pending_start_req_[evse] = transaction_module2_0::PendingStartRequest {
                        platform_,
                        evse,
                        ocpp2_0::IdTokenType {
                                plug_and_charge_id,
                                ocpp2_0::IdTokenEnumType::kLocal
                        }
                };
            }
        }

        void processPendingStartRequest(ocpp2_0::OcppRemote& remote, std::optional<transaction_module2_0::PendingStartRequest>& pending) {
            if (!pending.has_value())
                return;

            // If the request has expired remove it
            auto const elapsed = platform_->steadyClockNow() - pending->created_timestamp;
            if (elapsed >= settings_->ConnectionTimeOut.getValue()*1000) {
                pending = std::nullopt;
                return;
            }

            // Attempt to authorize the request if necessary
            if (!pending->authorize_finished) {
                if (treatAsConnected()) {
                    // F01.FR.01
                    if (!pending->pending_authorize_req.operationInProgress()) {
                        pending->pending_authorize_req.setWithTimeout(
                                settings_->DefaultMessageTimeout.getValue(),
                                remote.sendAuthorizeReq(ocpp2_0::AuthorizeRequest {
                                        std::nullopt,
                                        pending->id_token
                                })
                        );
                    }

                    return;
                } else if (!settings_->AllowOfflineTxForUnknownId.getValue()) {
                    return;
                }
            }

            // First attempt to associate the ID tag with an existing transaction
            for (auto& active : active_transactions_) {
                if (!active.second.has_value())
                    continue;
                if (active.second->authorized)
                    continue;
                if (pending->evse.has_value()) {
                    if (!active.first.has_value())
                        continue;
                    if (pending->evse->id != active.first->id)
                        continue;
                    if (pending->evse->connectorId.has_value() && pending->evse->connectorId != active.first->connectorId)
                        continue;
                }

                active.second->authorized = true;
                updateTransaction(
                        active.first,
                        pending->id_token,
                        pending->group_id_token,
                        ocpp2_0::TriggerReasonEnumType::kAuthorized,
                        platform_->systemClockNow()
                );
                return;
            }

            // Don't attempt to start new transactions if a reset is pending
            if (connector_status_module_->getPendingReset())
                return;

            // Attempt to start a transaction against plugged in connectors
            std::set<int> evse_ids_with_active_transactions;
            for (auto const& x : active_transactions_) {
                if (x.first.has_value() && x.second.has_value())
                    evse_ids_with_active_transactions.insert(x.first->id);
            }

            auto const start_points = getStartPoints();
            for (auto const& entry : station_->getConnectorMetadata()) {
                if (evse_ids_with_active_transactions.find(entry.first.id) != evse_ids_with_active_transactions.end())
                    continue;

                auto const& status = station_->pollConnectorStatus(entry.first);
                if (!status.has_value() || !status->vehicle_connected)
                    continue;
                if (pending->evse.has_value()) {
                    if (pending->evse->id != entry.first.id)
                        continue;
                    if (pending->evse->connectorId.has_value() && pending->evse->connectorId.value() != entry.first.connectorId.value())
                        continue;
                }

                if (set::contains(start_points, ocpp2_0::TxStartPointValues::kPowerPathClosed)) {
                    // Start using the kEVConnected PowerPathClosed, if configured
                    // Note - the pending start request would be authorized at this point
                    ocpp2_0::TriggerReasonEnumType start_reason;
                    if (pending->remote_start_id.has_value()) {
                        // F02.FR.21
                        start_reason = ocpp2_0::TriggerReasonEnumType::kRemoteStart;
                    } else {
                        // TODO: Is this the right start reason?
                        start_reason = ocpp2_0::TriggerReasonEnumType::kCablePluggedIn;
                    }

                    auto const now = floorToSecond(platform_->systemClockNow());
                    auto const& result = startTransaction(
                            entry.first,
                            pending->id_token,
                            pending->group_id_token,
                            start_reason,
                            true,
                            now,
                            // E01.FR.09
                            getMeterValues(
                                    entry.first,
                                    ocpp2_0::ReadingContextEnumType::kTransaction_Begin,
                                    settings_->SampledDataTxStartedMeasurands.getValue(),
                                    now
                            ),
                            pending->charging_profile,
                            pending->remote_start_id
                    );
                    CHARGELAB_LOG_MESSAGE(info) << "Started new transaction (PowerPathClosed) - transaction ID: " << result.transaction_id;
                    pending = std::nullopt;
                    return;
                } else if (set::contains(start_points, ocpp2_0::TxStartPointValues::kAuthorized)) {
                    // Note - try to start an Authorized transaction here first to include the EVSE/connector ID.
                    // E01.FR.16
                    ocpp2_0::TriggerReasonEnumType start_reason;
                    if (pending->remote_start_id.has_value()) {
                        // F02.FR.21
                        start_reason = ocpp2_0::TriggerReasonEnumType::kRemoteStart;
                    } else {
                        start_reason = ocpp2_0::TriggerReasonEnumType::kAuthorized;
                    }

                    auto const now = floorToSecond(platform_->systemClockNow());
                    auto const& result = startTransaction(
                            entry.first,
                            pending->id_token,
                            pending->group_id_token,
                            start_reason,
                            true,
                            now,
                            // TODO: Should we be sending meter values here?
                            getMeterValues(
                                    entry.first,
                                    ocpp2_0::ReadingContextEnumType::kTransaction_Begin,
                                    settings_->SampledDataTxStartedMeasurands.getValue(),
                                    now
                            ),
                            pending->charging_profile,
                            pending->remote_start_id
                    );
                    CHARGELAB_LOG_MESSAGE(info) << "Started new transaction (Authorized with connector) - transaction ID: " << result.transaction_id;
                    pending = std::nullopt;
                    return;
                }
            }

            // If we weren't able to start an Authorized transaction against a plugged in connector, then start a
            // transaction without an assigned connector.
            if (set::contains(start_points, ocpp2_0::TxStartPointValues::kAuthorized)) {
                ocpp2_0::TriggerReasonEnumType start_reason;
                if (pending->remote_start_id.has_value()) {
                    // F02.FR.21
                    start_reason = ocpp2_0::TriggerReasonEnumType::kRemoteStart;
                } else {
                    start_reason = ocpp2_0::TriggerReasonEnumType::kAuthorized;
                }

                auto const now = floorToSecond(platform_->systemClockNow());
                auto const& result = startTransaction(
                        pending->evse,
                        pending->id_token,
                        pending->group_id_token,
                        start_reason,
                        true,
                        now,
                        std::nullopt,
                        pending->charging_profile,
                        pending->remote_start_id
                );
                CHARGELAB_LOG_MESSAGE(info) << "Started new transaction (Authorized without connector) - transaction ID: " << result.transaction_id;
                pending = std::nullopt;
                return;
            }
        }

        void addSampledReading(
                SystemTimeMillis now,
                std::optional<ocpp2_0::EVSEType> const& evse,
                transaction_module2_0::TransactionContainer& entry,
                ocpp2_0::TriggerReasonEnumType const& trigger_reason,
                ocpp2_0::ReadingContextEnumType const& context,
                std::string const& measurands
        ) {
            if (!evse.has_value()) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed adding reading - no associated evse";
                return;
            }

            // TODO: It may be helpful to batch these while offline so that fewer requests are sent when the station
            //  reconnects instead of persisting individual samples.
            pending_messages_module_->sendRequest2_0(
                    ocpp2_0::TransactionEventRequest {
                            ocpp2_0::TransactionEventEnumType::kUpdated,
                            now,
                            trigger_reason,
                            0,
                            !isWebsocketConnected(),
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::TransactionType {
                                    std::to_string(entry.transaction_id),
                                    entry.last_charging_status = getChargingState(evse, entry)
                            },
                            std::nullopt,
                            evse,
                            getMeterValues(evse, context, measurands, now)
                    },
                    PendingMessagePolicy {
                            PendingMessageType::kTransactionEvent,
                            entry.transaction_id,
                            settings_->TransactionMessageAttempts.getValue(),
                            settings_->TransactionMessageRetryInterval.getValue(),
                            kPriorityMeterValue,
                            false,
                            true
                    }
            );
        }

        void addEndedReading(
                SystemTimeMillis now,
                std::optional<ocpp2_0::EVSEType> const& evse,
                transaction_module2_0::TransactionContainer& entry,
                ocpp2_0::ReadingContextEnumType const& context,
                std::string const& measurands
        ) {
            auto const meter_values = getMeterValues(evse, context, measurands, now);
            if (!meter_values.has_value())
                return;

            for (auto const& x : meter_values.value())
                entry.ended_meter_values.push_back(x);

            std::size_t total_size;
            while (true) {
                total_size = sizeof(entry.ended_meter_values);
                for (auto const& meter_value : entry.ended_meter_values) {
                    total_size += sizeof(meter_value);
                    for (auto const &sampled_value: meter_value.sampledValue)
                        total_size += sizeof(sampled_value);
                }

                if ((int)total_size <= settings_->MaxTransactionEndedMeterValuesBytes.getValue())
                    break;

                std::uniform_int_distribution<int64_t> distribution(0, entry.ended_meter_values.size() - 1);
                auto const it = entry.ended_meter_values.begin() + distribution(random_engine_);
                CHARGELAB_LOG_MESSAGE(info) << "Dropping ended MeterValue - MaxTransactionEndedMeterValuesBytes limit exceeded: " << *it;
                entry.ended_meter_values.erase(it);
            }

            CHARGELAB_LOG_MESSAGE(debug) << "Total ended meter values size: " << total_size << " bytes";
        }

        bool shouldTakeReading(SystemTimeMillis& last, int interval_seconds) {
            // Treat a configured interval of 0 (or negative) as disabled
            if (interval_seconds <= 0)
                return false;

            auto const now = platform_->systemClockNow();
            auto const delta_seconds = (now - last)/1000;
            if (delta_seconds < 0) {
                if (delta_seconds >= -kClockDriftThresholdSeconds)
                    return false;

                CHARGELAB_LOG_MESSAGE(warning) << "Negative elapsed interval - taking reading: " << delta_seconds;
            }

            SystemTimeMillis ts;
            if (delta_seconds < 0) {
                auto const intervals = -delta_seconds/interval_seconds;
                ts = static_cast<SystemTimeMillis>(last - (intervals+1)*interval_seconds*1000);
            } else {
                auto const intervals = delta_seconds/interval_seconds;
                ts = static_cast<SystemTimeMillis>(last + intervals*interval_seconds*1000);
            }

            if (last == ts)
                return false;

            last = ts;
            return true;
        }

        ocpp2_0::ChargingStateEnumType getChargingState(
                std::optional<ocpp2_0::EVSEType> evse_id,
                transaction_module2_0::TransactionContainer& transaction
        ) {
            if (!evse_id.has_value())
                return ocpp2_0::ChargingStateEnumType::kIdle;

            auto const& status = station_->pollConnectorStatus(evse_id.value());
            if (!status.has_value())
                return ocpp2_0::ChargingStateEnumType::kIdle;
            if (!status->vehicle_connected)
                return ocpp2_0::ChargingStateEnumType::kIdle;
            if (!transaction.authorized || transaction.deauthorized)
                return ocpp2_0::ChargingStateEnumType::kEVConnected;

            // TODO: Is this the right status to return here? For example: in the interval between EV plugged in and
            //       transaction authorized?
            if (!status->charging_enabled)
                return ocpp2_0::ChargingStateEnumType::kEVConnected;

            // TODO: Does the specification comment on which should take precedence here?
            if (status->suspended_by_charger)
                return ocpp2_0::ChargingStateEnumType::kSuspendedEVSE;
            if (status->suspended_by_vehicle)
                return ocpp2_0::ChargingStateEnumType::kSuspendedEV;

            return ocpp2_0::ChargingStateEnumType::kCharging;
        }

        bool treatAsConnected() {
            if (!boot_notification_module_->registrationComplete())
                return false;
            
            auto const websocket = platform_->ocppConnection();
            if (websocket == nullptr)
                return false;
            
            return websocket->isConnected();
        }

        bool isWebsocketConnected() {
            auto const websocket = platform_->ocppConnection();
            if (websocket == nullptr)
                return false;

            return websocket->isConnected();
        }

        std::optional<std::vector<ocpp2_0::MeterValueType>> getMeterValues(
                std::optional<ocpp2_0::EVSEType> const& evse,
                ocpp2_0::ReadingContextEnumType const& context,
                std::string const& measurands,
                SystemTimeMillis now
        ) {
            if (!evse.has_value() || measurands.empty())
                return std::nullopt;

            auto enabled = parseMeasurandsString(measurands);
            auto sampled_values = station_->pollMeterValues2_0(evse.value());
            sampled_values.erase(
                    std::remove_if(
                            sampled_values.begin(),
                            sampled_values.end(),
                            [&](ocpp2_0::SampledValueType const& value) {
                                auto measurand = value.measurand.value_or(ocpp2_0::MeasurandEnumType::kEnergy_Active_Import_Register);
                                return enabled.find(measurand) == enabled.end();
                            }
                    ),
                    sampled_values.end()
            );

            if (sampled_values.empty())
                return std::nullopt;

            // Update the context
            for (auto& value : sampled_values)
                value.context = context;

            return std::vector<ocpp2_0::MeterValueType> {ocpp2_0::MeterValueType {now, sampled_values}};
        }

        std::set<ocpp2_0::TxStartPointValues> getStartPoints() {
            std::set<ocpp2_0::TxStartPointValues> result;
            string::SplitVisitor(settings_->TxStartPoint.getValue(), ",", [&](std::string const& value) {
                auto value_as_enum = ocpp2_0::TxStartPointValues::from_string(value);
                if (value_as_enum == ocpp2_0::TxStartPointValues::kValueNotFoundInEnum) {
                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected start point: " << value;
                } else {
                    result.insert(value_as_enum);
                }
            });

            return result;
        }

        std::set<ocpp2_0::TxStopPointValues> getStopPoints() {
            std::set<ocpp2_0::TxStopPointValues> result;
            string::SplitVisitor(settings_->TxStopPoint.getValue(), ",", [&](std::string const& value) {
                auto value_as_enum = ocpp2_0::TxStopPointValues::from_string(value);
                if (value_as_enum == ocpp2_0::TxStopPointValues::kValueNotFoundInEnum) {
                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected start point: " << value;
                } else {
                    result.insert(value_as_enum);
                }
            });

            return result;
        }
        
        static std::unordered_set<ocpp2_0::MeasurandEnumType::Value> parseMeasurandsString(std::string const& measurands) {
            // comma separated list, e.g. "Current.Import,Energy.Active.Import.Register"
            std::unordered_set<ocpp2_0::MeasurandEnumType::Value> ret;
            string::SplitVisitor(measurands, ",", [&](std::string const& value) {
                auto value_as_enum = ocpp2_0::MeasurandEnumType::from_string(value);
                if (value_as_enum == ocpp2_0::MeasurandEnumType::kValueNotFoundInEnum) {
                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected measurand: " << value;
                } else {
                    ret.insert(value_as_enum);
                }
            });

            return ret;
        }

        static SystemTimeMillis floorToSecond(SystemTimeMillis ts) {
            return static_cast<SystemTimeMillis>((ts/1000)*1000);
        }

    //private:
    public:
        std::shared_ptr<PlatformInterface> platform_;
        std::shared_ptr<BootNotificationModule> boot_notification_module_;
        std::shared_ptr<PowerManagementModule2_0> power_management_module_;
        std::shared_ptr<PendingMessagesModule> pending_messages_module_;
        std::shared_ptr<ConnectorStatusModule> connector_status_module_;
        std::shared_ptr<StationInterface> station_;
        std::default_random_engine random_engine_;
        std::shared_ptr<PendingMessagesModule::saved_message_supplier> stop_transaction_supplier_;

        std::shared_ptr<Settings> settings_;
        std::atomic<int64_t> unique_index_;
        std::map<std::optional<ocpp2_0::EVSEType>, std::optional<transaction_module2_0::PendingStartRequest>> pending_start_req_;
        std::map<ocpp2_0::EVSEType, bool> last_plugged_in_state_;
        std::optional<ocpp2_0::IdTokenType> last_rfid_tag_id_ = std::nullopt;
        SystemTimeMillis last_non_transaction_meter_value_trigger_ = static_cast<SystemTimeMillis> (0);

        // Note: a transaction will remain here until the connector is unplugged or a new transaction starts
        std::map<std::optional<ocpp2_0::EVSEType>, std::optional<transaction_module2_0::TransactionContainer>> active_transactions_;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_TRANSACTION_MODULE2_0_H
