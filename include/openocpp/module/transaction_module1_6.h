#ifndef CHARGELAB_OPEN_FIRMWARE_TRANSACTION_MODULE1_6_H
#define CHARGELAB_OPEN_FIRMWARE_TRANSACTION_MODULE1_6_H

#include "openocpp/interface/platform_interface.h"
#include "openocpp/interface/station_interface.h"
#include "openocpp/module/common_templates.h"
#include "openocpp/module/pending_messages_module.h"
#include "openocpp/module/boot_notification_module.h"
#include "openocpp/module/power_management_module1_6.h"
#include "openocpp/module/connector_status_module.h"
#include "openocpp/common/operation_holder.h"
#include "openocpp/common/logging.h"

#include <utility>
#include <random>
#include <unordered_set>
#include <deque>
#include <vector>
#include <memory>
#include <string>
#include <optional>

namespace chargelab {
    namespace transaction_module1_6 {
        struct TransactionContainer {
            int connector_id;
            uint64_t group_id;
            std::string id_tag;
            std::string start_transaction_request_id;

            std::optional<int> transaction_id = std::nullopt;
            std::optional<SteadyPointMillis> last_reading_timestamp = std::nullopt;
            std::optional<SteadyPointMillis> last_clock_aligned_reading_timestamp = std::nullopt;
            bool connector_unplugged_after_transaction_ended = false;
        };

        struct PendingStartRequest {
            PendingStartRequest(std::shared_ptr<PlatformInterface> const& platform, int connector_id, std::string tag_id)
                : created_timestamp {platform->steadyClockNow()},
                  pending_authorize_req {platform},
                  connector_id {connector_id},
                  tag_id {tag_id},
                  charging_profile {},
                  authorize_finished {false}
            {
            }

            PendingStartRequest(std::shared_ptr<PlatformInterface> const& platform, ocpp1_6::RemoteStartTransactionReq const& req)
                : created_timestamp {platform->steadyClockNow()},
                  pending_authorize_req {platform},
                  connector_id {req.connectorId.value_or(0)},
                  tag_id {req.idTag.value()},
                  charging_profile {req.chargingProfile},
                  authorize_finished {!platform->getSettings()->AuthorizeRemoteTxRequests.getValue()}
            {
            }

            SteadyPointMillis created_timestamp;
            OperationHolder<std::string> pending_authorize_req;
            int connector_id;
            std::string tag_id;
            std::optional<ocpp1_6::ChargingProfile> charging_profile;

            bool authorize_finished;
        };
    }

    class TransactionModule1_6 : public ServiceStateful1_6 {
    private:
        static constexpr int kPriorityStartTransaction = 0;
        static constexpr int kPriorityStopTransaction = 0;
        static constexpr int kPriorityMeterValue = 5;

        static const int kMaxReadingsBytes = 10*1024;
        static const int kGeneralOperationMaxFailures = 4;
        static constexpr int kOfflineTransactions = 3;
        static constexpr char const* kDefaultTagId = "missing-tag";

        static constexpr int kTransactionMessageRetryIntervalSeconds = 10;
        static const int kSecondsPerDay = 86400; // 24*60*60
    public:
        TransactionModule1_6(
                std::shared_ptr<PlatformInterface> const& platform,
                std::shared_ptr<BootNotificationModule> boot_notification_module,
                std::shared_ptr<PowerManagementModule1_6> power_management_module,
                std::shared_ptr<PendingMessagesModule> pending_messages_module,
                std::shared_ptr<ConnectorStatusModule> connector_status_module,
                std::shared_ptr<StationInterface> station
        ) : platform_(platform),
            boot_notification_module_(std::move(boot_notification_module)),
            power_management_module_(std::move(power_management_module)),
            pending_messages_module_(std::move(pending_messages_module)),
            connector_status_module_(std::move(connector_status_module)),
            station_(std::move(station))
        {
            settings_ = platform_->getSettings();

            std::default_random_engine random_engine {std::random_device{}()};
            std::uniform_int_distribution<int64_t> distribution(
                    std::numeric_limits<int64_t>::min(),
                    std::numeric_limits<int64_t>::max()
            );
            unique_index_ = distribution(random_engine);

            stop_transaction_supplier_ = std::make_shared<PendingMessagesModule::saved_message_supplier> (
                    [&](std::function<void(detail::PendingMessageWrapper const&)> const& processor) {
                        for (auto& entry : active_transactions_) {
                            if (!entry.second.has_value())
                                continue;

                            auto const request = generateStopRequest(entry.first, ocpp1_6::Reason::kPowerLoss);
                            if (request.has_value())
                                processor(request.value());
                        }
                    }
            );
            pending_messages_module_->registerOnSaveMessageSupplier(stop_transaction_supplier_);
        }

        ~TransactionModule1_6() override {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting TransactionModule1_6";
        }

    private:
        void runStep(ocpp1_6::OcppRemote& remote) override {
            // If a reset was requested stop all active transactions
            if (connector_status_module_->getPendingReset()) {
                for (auto& x : active_transactions_) {
                    if (!x.second.has_value())
                        continue;

                    CHARGELAB_LOG_MESSAGE(info) << "Stopping transaction (pending reset) - transaction ID: " << x.second->transaction_id;
                    stopTransaction(x.first, connector_status_module_->getResetReasonHard() ? ocpp1_6::Reason::kHardReset : ocpp1_6::Reason::kSoftReset);
                }
                return; // We should not handle any other events when reset is pending.
            }

            // Process an RFID tap if there was a state change
            auto id_token = station_->readToken1_6();
            if (id_token != last_rfid_tag_id_) {
                last_rfid_tag_id_ = id_token;
                if (id_token.has_value())
                    processRfidTap(id_token->value());
            }

            // Process plugged in state changes (PlugAndCharge and stopping transaction on disconnect)
            for (auto const& entry: station_->getConnectorMetadata()) {
                // Only take action if the connector state has changed. If, for example, a pending RemoteStarTransaction
                // request timed out do not attempt an autostart transaction until the cable is unplugged and plugged
                // back in.
                auto const connector_status = station_->pollConnectorStatus(entry.first);
                if (!connector_status.has_value())
                    continue;

                auto const id = entry.second.connector_id1_6;
                if (connector_status->vehicle_connected == last_plugged_in_state_[id])
                    continue;

                last_plugged_in_state_[id] = connector_status->vehicle_connected;
                processVehicleConnectedStateChanged(id, connector_status->vehicle_connected);
            }

            // First process any pending start requests for a specific connector, then process any for the station
            for (auto& entry : pending_start_req_) {
                if (entry.first > 0)
                    processPendingStartRequest(remote, entry.second);
            }
            for (auto& entry : pending_start_req_) {
                if (entry.first <= 0)
                    processPendingStartRequest(remote, entry.second);
            }

            std::vector<std::optional<ocpp2_0::EVSEType>> pending;

            for (auto& entry : pending_start_req_) {
                if (entry.second.has_value()) {
                    if (entry.first > 0) {
                        pending.emplace_back(ocpp2_0::EVSEType {entry.first, 1});
                    } else if (entry.first == 0 && entry.second != std::nullopt) {
                        pending.emplace_back(std::nullopt);
                    }
                }
            }

            connector_status_module_->setPendingStartRequests(pending);

            // Process transactions
            for (auto& entry : active_transactions_) {
                if (!entry.second.has_value())
                    continue;

                auto const evse = station_->lookupConnectorId1_6(entry.first);
                if (!evse.has_value()) {
                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected state: connector did not exist for running transaction - dropping: " << entry.second->transaction_id;
                    entry.second = std::nullopt;
                    continue;
                }

                bool first_reading = !entry.second->last_reading_timestamp.has_value();
                if (shouldTakeReading(entry.second->last_reading_timestamp, settings_->MeterValueSampleInterval.getValue()))
                    addReading(entry.second.value(), first_reading ? ocpp1_6::ReadingContext::kTransactionBegin : ocpp1_6::ReadingContext::kSamplePeriodic);

                first_reading = !entry.second->last_clock_aligned_reading_timestamp.has_value();
                if (shouldTakeReading(entry.second->last_clock_aligned_reading_timestamp, settings_->ClockAlignedDataInterval.getValue()))
                    addReading(entry.second.value(), first_reading ? ocpp1_6::ReadingContext::kTransactionBegin : ocpp1_6::ReadingContext::kSampleClock);
            }

#if 0        // For OCTT _012
            for (auto it = pending_stop_transaction_times_.begin(); it != pending_stop_transaction_times_.end(); ) {
                if (platform_->steadyClockNow() -  it->second > 5000 ) {
                    stopTransaction(it->first, ocpp1_6::Reason::kRemote);
                    it = pending_stop_transaction_times_.erase(it);  // erase returns the next iterator
                } else {
                    ++it;
                }
            }
#endif
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::RemoteStartTransactionRsp>> onRemoteStartTransactionReq(
                const ocpp1_6::RemoteStartTransactionReq &req
        ) override {
            int connector_id = 0;
            if (req.connectorId.has_value()) {
                connector_id = req.connectorId.value();

                bool found = false;
                for (auto const& entry : station_->getConnectorMetadata()) {
                    if (entry.second.connector_id1_6 == connector_id) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    return ocpp1_6::RemoteStartTransactionRsp{ocpp1_6::RemoteStartStopStatus::kRejected};
                }
            }

            if (req.chargingProfile.has_value()) {
                // Note: as per 5.16.2. the profile purpose must be TxProfile
                if (req.chargingProfile->chargingProfilePurpose != ocpp1_6::ChargingProfilePurposeType::kTxProfile)
                    return ocpp1_6::RemoteStartTransactionRsp {ocpp1_6::RemoteStartStopStatus::kRejected};

                // Note: as per 5.16.2. the transaction ID must not be set
                if (req.chargingProfile->transactionId.has_value())
                    return ocpp1_6::RemoteStartTransactionRsp {ocpp1_6::RemoteStartStopStatus::kRejected};
            }

            bool available_connector = true;
            // check if there is an active transaction already
            for (auto& x : active_transactions_) {
                if (req.connectorId && req.connectorId == x.first && x.second) {
                    available_connector = false;
                    break;
                }
            }

            if (!available_connector) {
                return ocpp1_6::RemoteStartTransactionRsp{ocpp1_6::RemoteStartStopStatus::kRejected};
            }

            // Note: adopting the convention that a new RemoteStartTransaction request will replace an existing pending
            // one, rather than be blocked by it.
            pending_start_req_[connector_id] = transaction_module1_6::PendingStartRequest {platform_, req};
            return ocpp1_6::RemoteStartTransactionRsp {ocpp1_6::RemoteStartStopStatus::kAccepted};
        }

        void onAuthorizeRsp(
                const std::string &unique_id,
                const std::variant<ocpp1_6::AuthorizeRsp, ocpp1_6::CallError> &rsp
        ) override {
            for (auto& entry : pending_start_req_) {
                if (!entry.second.has_value())
                    continue;
                if (entry.second->pending_authorize_req != unique_id)
                    continue;

                if (std::holds_alternative<ocpp1_6::AuthorizeRsp>(rsp)) {
                    // Note: only clearing the operation if a valid response was received, otherwise treating as absent
                    // and retrying after configured timeout.
                    entry.second->pending_authorize_req = kNoOperation;

                    auto const& value = std::get<ocpp1_6::AuthorizeRsp>(rsp);
                    CHARGELAB_LOG_MESSAGE(info) << "Received authorize response: " << value;
                    switch (value.idTagInfo.status) {
                        case ocpp1_6::AuthorizationStatus::kAccepted:
                            entry.second->authorize_finished = true;
                            CHARGELAB_LOG_MESSAGE(info) << "Set authorize_finished to true: " << entry.second->authorize_finished;
                            break;

                        case ocpp1_6::AuthorizationStatus::kValueNotFoundInEnum:
                        case ocpp1_6::AuthorizationStatus::kBlocked:
                        case ocpp1_6::AuthorizationStatus::kExpired:
                        case ocpp1_6::AuthorizationStatus::kInvalid:
                        case ocpp1_6::AuthorizationStatus::kConcurrentTx:
                            entry.second = std::nullopt;
                            break;
                    }
                } else {
                    CHARGELAB_LOG_MESSAGE(warning) << "Error response to Authorize request: " << std::get<ocpp1_6::CallError> (rsp);
                }
            }
        }

        void onStartTransactionRsp(
                const std::string &unique_id,
                const std::variant<ocpp1_6::StartTransactionRsp, ocpp1_6::CallError> &rsp
        ) override {
            // Note: this is processed here in addition to pending_messages to allow this module to take action if the
            // back-end does not accept the provided idTag.
            for (auto& entry : active_transactions_) {
                if (!entry.second.has_value())
                    continue;
                if (unique_id != entry.second->start_transaction_request_id)
                    continue;

                if (std::holds_alternative<ocpp1_6::StartTransactionRsp>(rsp)) {
                    auto const &value = std::get<ocpp1_6::StartTransactionRsp>(rsp);
                    entry.second->transaction_id = value.transactionId;

                    switch (value.idTagInfo.status) {
                        case ocpp1_6::AuthorizationStatus::kAccepted:
                            power_management_module_->onActiveTransactionIdAssigned(entry.first, value.transactionId);
                            break;

                        case ocpp1_6::AuthorizationStatus::kValueNotFoundInEnum:
                        case ocpp1_6::AuthorizationStatus::kBlocked:
                        case ocpp1_6::AuthorizationStatus::kExpired:
                        case ocpp1_6::AuthorizationStatus::kInvalid:
                        case ocpp1_6::AuthorizationStatus::kConcurrentTx:
                            // TODO: Support MaxEnergyOnInvalidId?
                            stopTransaction(entry.first, ocpp1_6::Reason::kDeAuthorized);
                            break;
                    }
                } else {
                    CHARGELAB_LOG_MESSAGE(warning) << "Error response to StartTransaction request: " << std::get<ocpp1_6::CallError>(rsp);
                }
            }
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::RemoteStopTransactionRsp>> onRemoteStopTransactionReq(
                const ocpp1_6::RemoteStopTransactionReq &req
        ) override {
            for (auto& entry : active_transactions_) {
                if (!entry.second.has_value())
                    continue;
                if (!entry.second->transaction_id.has_value())
                    continue;
                if (entry.second->transaction_id.value() != req.transactionId)
                    continue;

#if 0        // For OCTT _012
                pending_stop_transaction_times_[entry.first] = platform_->steadyClockNow();
#else
                stopTransaction(entry.first, ocpp1_6::Reason::kRemote);
#endif
                return ocpp1_6::RemoteStopTransactionRsp {ocpp1_6::RemoteStartStopStatus::kAccepted};
            }

            // TODO: unrecognized transaction module behaves badly with this one. Maybe fold that implementation into
            //  this one?
            //return ocpp1_6::RemoteStopTransactionRsp {ocpp1_6::RemoteStartStopStatus::kRejected};

            if (!settings_->StopTransactionWithDifferentId.getValue())
                return ocpp1_6::RemoteStopTransactionRsp {ocpp1_6::RemoteStartStopStatus::kRejected};
            if (!force_stop_transaction_.has_value()) {
                force_stop_transaction_ = req.transactionId;
                return ocpp1_6::RemoteStopTransactionRsp {ocpp1_6::RemoteStartStopStatus::kAccepted};
            } else {
                return ocpp1_6::RemoteStopTransactionRsp {ocpp1_6::RemoteStartStopStatus::kRejected};
            }

            return std::nullopt;
        }

        std::optional<ocpp1_6::ResponseToRequest <ocpp1_6::TriggerMessageRsp>>
        onTriggerMessageReq(const ocpp1_6::TriggerMessageReq &req) override {
            if (req.requestedMessage != ocpp1_6::MessageTrigger::kMeterValues)
                return std::nullopt;

            for (auto& entry : active_transactions_) {
                if (!entry.second.has_value())
                    continue;
                if (req.connectorId.has_value() && req.connectorId.value() != entry.first)
                    continue;

                addReading(entry.second.value(), ocpp1_6::ReadingContext::kTrigger);
                return ocpp1_6::TriggerMessageRsp {ocpp1_6::TriggerMessageStatus::kAccepted};
            }

            return ocpp1_6::TriggerMessageRsp {ocpp1_6::TriggerMessageStatus::kRejected};
        }

    private:
        void startTransaction(
                int connector_id,
                std::string const& id_tag,
                std::optional<ocpp1_6::ChargingProfile> const& charging_profile = std::nullopt
        ) {
            auto const group_id = unique_index_++;
            auto const evse = station_->lookupConnectorId1_6(connector_id);
            if (!evse.has_value()) {
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected state - connector ID does not exist: " << connector_id;
                return;
            }

            auto const status = station_->pollConnectorStatus(evse.value());
            if (!status.has_value()) {
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected state - failed fetching connector status for EVSE: " << evse;
                return;
            }

            auto start_transaction_request_id = pending_messages_module_->sendRequest1_6(
                    ocpp1_6::StartTransactionReq {
                            connector_id,
                            id_tag,
                            (int)status->meter_watt_hours,
                            std::nullopt,
                            platform_->systemClockNow()
                    },
                    PendingMessagePolicy {
                            PendingMessageType::kTransactionEvent,
                            group_id,
                            settings_->TransactionMessageAttempts.getValue(),
                            settings_->TransactionMessageRetryInterval.getValue(),
                            kPriorityStartTransaction,
                            false,
                            false,
                            true
                    }
            );
            // Setting enabling charging earlier for OCTT test case _010
            station_->setChargingEnabled(evse.value(), true);

            active_transactions_[connector_id] = transaction_module1_6::TransactionContainer {
                    connector_id,
                    group_id,
                    id_tag,
                    start_transaction_request_id
            };

            power_management_module_->onActiveTransactionStarted(
                    connector_id,
                    charging_profile
            );
        }

        std::optional<detail::PendingMessageWrapper> generateStopRequest(int connector_id, ocpp1_6::Reason const& reason) {
            auto const& transaction = active_transactions_[connector_id];
            if (!transaction.has_value())
                return std::nullopt;

            auto const evse = station_->lookupConnectorId1_6(connector_id);
            if (!evse.has_value()) {
                // Note: the transaction is not stopped here intentionally; we'd have to provide a bad watt hour reading
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected state - connector ID does not exist: " << connector_id;
                return std::nullopt;
            }

            auto const status = station_->pollConnectorStatus(evse.value());

            if (!status.has_value()) {
                // Note: the transaction is not stopped here intentionally; we'd have to provide a bad watt hour reading
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected state - failed fetching connector status for EVSE: " << evse;
                return std::nullopt;
            }

            return pending_messages_module_->generateRequest1_6(
                    ocpp1_6::StopTransactionReq {
                            transaction->id_tag,
                            (int)status->meter_watt_hours,
                            platform_->systemClockNow(),
                            transaction->transaction_id.value_or(0),
                            reason,
                            {} // TODO: Stop transaction data
                    },
                    PendingMessagePolicy {
                            PendingMessageType::kTransactionEvent,
                            transaction->group_id,
                            settings_->TransactionMessageAttempts.getValue(),
                            settings_->TransactionMessageRetryInterval.getValue(),
                            kPriorityStopTransaction,
                            false,
                            false
                    }
            );
            power_management_module_->onActiveTransactionFinished(connector_id);
        }

        void stopTransaction(int connector_id, ocpp1_6::Reason const& reason) {
            std::optional<transaction_module1_6::TransactionContainer> transaction = std::nullopt;
            std::swap(transaction, active_transactions_[connector_id]);
            if (!transaction.has_value())
                return;

            auto const evse = station_->lookupConnectorId1_6(connector_id);
            if (!evse.has_value()) {
                // Note: the transaction is not stopped here intentionally; we'd have to provide a bad watt hour reading
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected state - connector ID does not exist: " << connector_id;
                return;
            }

            auto const status = station_->pollConnectorStatus(evse.value());
            station_->setChargingEnabled(evse.value(), false);

            if (!status.has_value()) {
                // Note: the transaction is not stopped here intentionally; we'd have to provide a bad watt hour reading
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected state - failed fetching connector status for EVSE: " << evse;
                return;
            }

            pending_messages_module_->sendRequest1_6(
                    ocpp1_6::StopTransactionReq {
                            transaction->id_tag,
                            (int)status->meter_watt_hours,
                            platform_->systemClockNow(),
                            transaction->transaction_id.value_or(0),
                            reason,
                            {} // TODO: Stop transaction data
                    },
                    PendingMessagePolicy {
                            PendingMessageType::kTransactionEvent,
                            transaction->group_id,
                            settings_->TransactionMessageAttempts.getValue(),
                            settings_->TransactionMessageRetryInterval.getValue(),
                            kPriorityStopTransaction,
                            false,
                            false,
                            true
                    }
            );
            power_management_module_->onActiveTransactionFinished(connector_id);
        }

        void processRfidTap(std::string const& id_tag) {
            // Check if a running transaction already uses this tag; if so it should be interpreted as a stop
            // request.
            for (auto& x : active_transactions_) {
                if (!x.second.has_value())
                    continue;
                if (string::EqualsIgnoreCaseAscii(x.second->id_tag, id_tag)) {
                    stopTransaction(x.first, ocpp1_6::Reason::kLocal);
                    return;
                }
            }

            // Need to check if there's availabe connector for another transaction, otherwise ignore the rfid tap event.
            // This is for the OCTT 1.6 test case _068 where a different RFID should not trigger the sending of the
            // Authorize request if there is an active transaction.
            bool found_available_connector = false;
            for (auto const& entry: station_->getConnectorMetadata()) {
                if (entry.first.id == 0) continue;
                auto const connector_status = station_->pollConnectorStatus(entry.first);
                if (!connector_status.has_value())
                    continue;
                if (connector_status->connector_available) {
                    found_available_connector = true;
                    break;
                }
            }

            if (!found_available_connector)
                return;

            // Add the PlugAndCharge entry
            pending_start_req_[0] = transaction_module1_6::PendingStartRequest{platform_, 0, id_tag};
        }

        void processVehicleConnectedStateChanged(int connector_id, bool connected) {
            if (active_transactions_[connector_id].has_value()) {
                if (!connected) {
                    stopTransaction(connector_id, ocpp1_6::Reason::kEVDisconnected);
                }

                // Note: PlugAndCharge should only trigger if there's no activate transaction.
                return;
            }

            auto plug_and_charge_id = settings_->PlugAndChargeId.getValue();
            if (connected && !plug_and_charge_id.empty()) {
                // If there's another potentially applicable start operation pending (remote start request, RFID tap,
                // etc) it should take precedence and autostart should be ignored.
                if (pending_start_req_[0].has_value() || pending_start_req_[connector_id].has_value())
                    return;

                // Add the PlugAndCharge entry
                pending_start_req_[connector_id] = transaction_module1_6::PendingStartRequest {
                        platform_,
                        connector_id,
                        plug_and_charge_id
                };
            }
        }

        void processPendingStartRequest(ocpp1_6::OcppRemote& remote, std::optional<transaction_module1_6::PendingStartRequest>& pending) {
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
                    if (!pending->pending_authorize_req.operationInProgress()) {
                        pending->pending_authorize_req.setWithTimeout(
                                settings_->DefaultMessageTimeout.getValue(),
                                remote.sendAuthorizeReq(ocpp1_6::AuthorizeReq {pending->tag_id})
                        );
                    }

                    return;
                } else if (!settings_->AllowOfflineTxForUnknownId.getValue()) {
                    return;
                }
            }

            // Attempt to start a transaction
            for (auto const& entry : station_->getConnectorMetadata()) {
                auto const id = entry.second.connector_id1_6;
                if (pending->connector_id != 0 && pending->connector_id != id)
                    continue;

                auto const status = station_->pollConnectorStatus(entry.first);
                if (!status.has_value() || !status->vehicle_connected)
                    continue;
                if (active_transactions_[id].has_value())
                    continue;

                // Start new transaction
                startTransaction(id, pending->tag_id, pending->charging_profile);
                pending = std::nullopt;
            }
        }

        void addReading(
                transaction_module1_6::TransactionContainer& entry,
                ocpp1_6::ReadingContext const& context
        ) {
            auto const evse = station_->lookupConnectorId1_6(entry.connector_id);
            if (!evse.has_value()) {
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected state - connector ID does not exist: " << entry.connector_id;
                return;
            }

            auto enabled = parseMeasurandsString(settings_->MeterValuesSampledData.getValue());
            auto sampled_values = station_->pollMeterValues1_6(evse.value());
            sampled_values.erase(
                    std::remove_if(
                            sampled_values.begin(),
                            sampled_values.end(),
                            [&](ocpp1_6::SampledValue const& value) {
                                auto measurand = value.measurand.value_or(ocpp1_6::Measurand::kEnergyActiveImportRegister);
                                return enabled.find(measurand) == enabled.end();
                            }
                    ),
                    sampled_values.end()
            );

            // Update the context
            for (auto& value : sampled_values)
                value.context = context;

            // TODO: It may be helpful to batch these while offline so that fewer requests are sent when the station
            //  reconnects instead of persisting individual samples.
            pending_messages_module_->sendRequest1_6(
                    ocpp1_6::MeterValuesReq {
                            entry.connector_id,
                            entry.transaction_id.value_or(0),
                            {
                                    ocpp1_6::MeterValue {
                                            platform_->systemClockNow(),
                                            std::move(sampled_values)
                                    }
                            }
                    },
                    PendingMessagePolicy {
                            PendingMessageType::kTransactionEvent,
                            entry.group_id,
                            settings_->TransactionMessageAttempts.getValue(),
                            settings_->TransactionMessageRetryInterval.getValue(),
                            kPriorityMeterValue,
                            false,
                            false
                    }
            );
        }

        bool shouldTakeReading(std::optional<SteadyPointMillis>& last, int interval_seconds) {
            // Treat a configured interval of 0 (or negative) as disabled
            if (interval_seconds <= 0)
                return false;

            auto const now = platform_->steadyClockNow();
            if (last.has_value()) {
                auto const delta = now - last.value();
                if (delta < interval_seconds*1000)
                    return false;
            }

            last = now;
            return true;
        }

        bool treatAsConnected() {
            if (!boot_notification_module_->registrationComplete())
                return false;
            
            auto const websocket = platform_->ocppConnection();
            if (websocket == nullptr)
                return false;
            
            return websocket->isConnected();
        }
        
        static std::unordered_set<ocpp1_6::Measurand::Value> parseMeasurandsString(std::string const& measurands) {
            // comma separated list, e.g. "Current.Import,Energy.Active.Import.Register"
            std::unordered_set<ocpp1_6::Measurand::Value> ret;
            string::SplitVisitor(measurands, ",", [&](std::string const& value) {
                auto value_as_enum = ocpp1_6::Measurand::from_string(value);
                if (value_as_enum == ocpp1_6::Measurand::kValueNotFoundInEnum) {
                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected measurand: " << value;
                } else {
                    ret.insert(value_as_enum);
                }
            });

            return ret;
        }

    private:
        std::shared_ptr<PlatformInterface> platform_;
        std::shared_ptr<BootNotificationModule> boot_notification_module_;
        std::shared_ptr<PowerManagementModule1_6> power_management_module_;
        std::shared_ptr<PendingMessagesModule> pending_messages_module_;
        std::shared_ptr<ConnectorStatusModule> connector_status_module_;
        std::shared_ptr<StationInterface> station_;
        std::shared_ptr<PendingMessagesModule::saved_message_supplier> stop_transaction_supplier_;

        std::shared_ptr<Settings> settings_;
        std::atomic<uint64_t> unique_index_;
        // key: connector ID
        std::unordered_map<int, std::optional<transaction_module1_6::PendingStartRequest>> pending_start_req_;
        // key: connector ID
        std::unordered_map<int, bool> last_plugged_in_state_;
        std::optional<ocpp1_6::IdToken> last_rfid_tag_id_ = std::nullopt;

        // Note: a transaction will remain here until the connector is unplugged or a new transaction starts
        // key: connector ID
        std::unordered_map<int, std::optional<transaction_module1_6::TransactionContainer>> active_transactions_;

        std::optional<int> connector_hold_id_ {std::nullopt};
        std::optional<int> force_stop_transaction_ = std::nullopt;

#if 0        // For OCTT _012
        std::unordered_map<int, SteadyPointMillis> pending_stop_transaction_times_;
#endif
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_TRANSACTION_MODULE1_6_H
