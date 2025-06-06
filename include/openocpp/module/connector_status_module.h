#ifndef CHARGELAB_OPEN_FIRMWARE_CONNECTOR_STATUS_MODULE_H
#define CHARGELAB_OPEN_FIRMWARE_CONNECTOR_STATUS_MODULE_H

#include "openocpp/module/common_templates.h"
#include "openocpp/interface/platform_interface.h"
#include "openocpp/interface/station_interface.h"
#include "openocpp/common/settings.h"
#include "openocpp/common/logging.h"
#include "openocpp/common/operation_holder.h"

#include <utility>
#include <set>

namespace chargelab {
    namespace detail {
        struct ReportedConnectorStatus {
            std::optional<SteadyPointMillis> last_status_sent = std::nullopt;
            std::optional<ocpp2_0::StatusNotificationRequest> last_reported_status2_0 = std::nullopt;
            std::optional<ocpp1_6::StatusNotificationReq> last_reported_status1_6 = std::nullopt;
            bool current_plug_was_charging = false;
            bool force_update = false;
        };
    }

    class ConnectorStatusModule : public ServiceStatefulGeneral {
    private:
        static constexpr int kSettingUpdateIntervalMillis = 500;

    public:
        ConnectorStatusModule(
                std::shared_ptr<Settings> settings,
                std::shared_ptr<PlatformInterface> const& platform,
                std::shared_ptr<StationInterface> station
        )
                : settings_(std::move(settings)),
                  platform_(platform),
                  station_(std::move(station)),
                  pending_connector_status_req_ {platform}
        {
            assert(platform_ != nullptr);
            loadInoperativeConnectors();
        }

        ~ConnectorStatusModule() override {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting ConnectorStatusModule";
        }

    public:
        bool isChargingEnabled() {
            for (auto const& entry : station_->getConnectorMetadata()) {
                auto const current_state = station_->pollConnectorStatus(entry.first);
                if (current_state.has_value() && current_state->charging_enabled)
                    return true;
            }

            return false;
        }

        void setPendingReset(bool value, bool hard_reset) {
            pending_reset_ = value;
            reset_reason_hard_ = hard_reset;
        }

        bool getPendingReset() const {
            return pending_reset_;
        }

        bool getResetReasonHard() const {
            return reset_reason_hard_;
        }

        void setPendingStartRequests(std::vector<std::optional<ocpp2_0::EVSEType>> const& pending) {
            pending_start_.assign(pending.begin(), pending.end());
        }

        std::unordered_map<int, ocpp1_6::ChargePointStatus> getChargePointStatus1_6() {
            std::lock_guard<std::mutex> lock(last_charge_point_status_map1_6_mutex_);
            return last_charge_point_status_map1_6_;
        }

        bool setConnector0Inoperative(bool inoperative) {
            if (inoperative_connectors_[std::nullopt] != inoperative) {
                inoperative_connectors_[std::nullopt] = inoperative;
                return true;
            }
            return false;
        }

    private:
        void runUnconditionally() override {
            addAndUpdateStateSettings();

            auto const connection = platform_->ocppConnection();
            if (connection != nullptr && connection->isConnected()) {
                last_online_ = platform_->steadyClockNow();
            } else if (last_online_.has_value()) {
                // B04.FR.01
                auto const elapsed = platform_->steadyClockNow() - last_online_.value();
                if (elapsed/1000 >= settings_->OfflineThreshold.getValue()) {
                    bool forced_update = false;
                    for (auto& x : reported_status_) {
                        if (!x.second.force_update) {
                            x.second.force_update = true;
                            forced_update = true;
                        }
                    }

                    if (forced_update) {
                        CHARGELAB_LOG_MESSAGE(info) << "OfflineThreshold exceeded - forcing status notification updates on next connection";
                    }
                }
            }
        }

        void runStep(ocpp2_0::OcppRemote &remote) override {
            for (auto& entry : station_->getConnectorMetadata()) {
                auto const current_state = station_->pollConnectorStatus(entry.first);
                if (!current_state.has_value())
                    continue;

                auto& reported_status = reported_status_[entry.first];
                advanceConnectorState(reported_status, current_state.value());
                auto current_status = getStatus2_0(entry.first, current_state.value());

                if (pending_connector_status_req_.operationInProgress())
                    continue;

                if (!reported_status.force_update && reported_status.last_reported_status2_0.has_value()) {
                    if (current_status == reported_status.last_reported_status2_0->connectorStatus)
                        continue;

                    // Note: rate limiting charging status doesn't appear to be part of the 2.0.1 spec
                }

                reported_status.last_status_sent = platform_->steadyClockNow();
                pending_connector_status_update2_0_ = std::make_pair(entry.first, ocpp2_0::StatusNotificationRequest {
                        ocpp2_0::DateTime{platform_->systemClockNow()},
                        current_status,
                        entry.first.id,
                        entry.first.connectorId.value()
                });
                pending_connector_status_req_.setWithTimeout(
                        settings_->DefaultMessageTimeout.getValue(),
                        remote.sendStatusNotificationReq(pending_connector_status_update2_0_->second)
                );
            }
        }

        void onStatusNotificationRsp(
                const std::string &uniqueId,
                const ocpp2_0::ResponseMessage<ocpp2_0::StatusNotificationResponse> &rsp
        ) override {
            if (pending_connector_status_req_ == uniqueId) {
                // TODO: Allowing failed operations to timeout instead of retrying immediately. This might need more
                //  thought.
                //pending_connector_status_req_ = kNoOperation;

                if (std::holds_alternative<ocpp2_0::StatusNotificationResponse>(rsp)) {
                    if (pending_connector_status_update2_0_.has_value()) {
                        auto const& evse = pending_connector_status_update2_0_->first;
                        auto const& request = pending_connector_status_update2_0_->second;
                        reported_status_[evse].last_reported_status2_0 = request;
                        reported_status_[evse].force_update = false;
                    }

                    pending_connector_status_req_ = kNoOperation;
                } else {
                    CHARGELAB_LOG_MESSAGE(warning) << "Error response to StatusNotification request: " << std::get<ocpp2_0::CallError> (rsp);
                }

                pending_connector_status_update2_0_ = std::nullopt;
            }
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::TriggerMessageResponse>>
        onTriggerMessageReq(const ocpp2_0::TriggerMessageRequest &req) override {
            if (req.requestedMessage != ocpp2_0::MessageTriggerEnumType::kStatusNotification)
                return std::nullopt;

            // Note: deviating from F06.FR.12 and treating req.evse as a filter instead. If evse isn't specified all
            //       connector statuses are reported, if connector ID isn't specified all statuses for that EVSE ID are
            //       reported.
            bool found = false;
            for (auto const& entry : station_->getConnectorMetadata()) {
                if (req.evse.has_value()) {
                    if (req.evse->id != entry.first.id)
                        continue;
                    if (req.evse->connectorId.has_value() && req.evse->connectorId != entry.first.connectorId)
                        continue;
                }

                CHARGELAB_LOG_MESSAGE(debug) << "Got trigger message for EVSE: " << entry.first;
                reported_status_[entry.first].force_update = true;
                found = true;
            }

            if (!found) {
                return ocpp2_0::TriggerMessageResponse {ocpp2_0::TriggerMessageStatusEnumType::kRejected};
            }

            return ocpp2_0::TriggerMessageResponse {ocpp2_0::TriggerMessageStatusEnumType::kAccepted};
        }

        void runStep(ocpp1_6::OcppRemote &remote) override {
            for (auto& entry : station_->getConnectorMetadata()) {
                auto const current_state = station_->pollConnectorStatus(entry.first);
                if (!current_state.has_value())
                    continue;

                auto& reported_status = reported_status_[entry.first];
                advanceConnectorState(reported_status, current_state.value());
                auto current_status = getStatus1_6(entry.first, current_state.value(), reported_status.current_plug_was_charging);

                if (pending_connector_status_req_.operationInProgress())
                    continue;

                {
                    std::lock_guard<std::mutex> lock(last_charge_point_status_map1_6_mutex_);
                    last_charge_point_status_map1_6_[entry.second.connector_id1_6] = current_status;
                }

                ocpp1_6::StatusNotificationReq request {
                        entry.second.connector_id1_6,
                        ocpp1_6::ChargePointErrorCode::kNoError,
                        std::nullopt,
                        current_status,
                        ocpp1_6::DateTime{platform_->systemClockNow()},
                        std::nullopt,
                        std::nullopt
                };

                if (current_state->faulted_status.has_value()) {
                    auto const& faulted = current_state->faulted_status->status1_6;
                    request.errorCode = faulted.errorCode;
                    request.info = faulted.info;
                    request.vendorId = faulted.vendorId;
                    request.vendorErrorCode = faulted.vendorErrorCode;
                }

                if (!reported_status.force_update && reported_status.last_reported_status1_6.has_value()) {
                    // Note: ignoring info parameter for updates; could potentially contain noisy information
                    auto const& last = reported_status.last_reported_status1_6.value();
                    if (request.status == last.status && request.errorCode == last.errorCode && request.vendorId == last.vendorId && request.vendorErrorCode == last.vendorErrorCode)
                        continue;

                    // TODO: rate limiting?
                }

                reported_status.last_status_sent = platform_->steadyClockNow();
                pending_connector_status_update1_6_ = std::make_pair(entry.first, request);
                pending_connector_status_req_.setWithTimeout(
                        settings_->DefaultMessageTimeout.getValue(),
                        remote.sendStatusNotificationReq(request)
                );
            }
        }

        void onStatusNotificationRsp(
                const std::string &uniqueId,
                const ocpp1_6::ResponseMessage<ocpp1_6::StatusNotificationRsp> &rsp
        ) override {
            if (pending_connector_status_req_ == uniqueId) {
                // TODO: Allowing failed operations to timeout instead of retrying immediately. This might need more
                //  thought.
                //pending_connector_status_req_ = kNoOperation;

                if (std::holds_alternative<ocpp1_6::StatusNotificationRsp>(rsp)) {
                    if (pending_connector_status_update1_6_.has_value()) {
                        auto const& evse = pending_connector_status_update1_6_->first;
                        auto const& request = pending_connector_status_update1_6_->second;
                        reported_status_[evse].last_reported_status1_6 = request;
                        reported_status_[evse].force_update = false;
                    }

                    pending_connector_status_req_ = kNoOperation;
                } else {
                    CHARGELAB_LOG_MESSAGE(warning) << "Error response to StatusNotification request: " << std::get<ocpp1_6::CallError> (rsp);
                }

                pending_connector_status_update1_6_ = std::nullopt;
            }
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::TriggerMessageRsp>>
        onTriggerMessageReq(const ocpp1_6::TriggerMessageReq &req) override {
            if (req.requestedMessage != ocpp1_6::MessageTrigger::kStatusNotification)
                return std::nullopt;

            bool found = false;
            for (auto const& entry : station_->getConnectorMetadata()) {
                if (req.connectorId.has_value()) {
                    if (entry.second.connector_id1_6 != req.connectorId.value())
                        continue;
                }

                CHARGELAB_LOG_MESSAGE(debug) << "Got trigger message for EVSE: " << entry.first;
                reported_status_[entry.first].force_update = true;
                found = true;
            }

            if (!found) {
                return ocpp1_6::TriggerMessageRsp {ocpp1_6::TriggerMessageStatus::kRejected};
            }

            return ocpp1_6::TriggerMessageRsp {ocpp1_6::TriggerMessageStatus::kAccepted};
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ChangeAvailabilityRsp>>
        onChangeAvailabilityReq(const ocpp1_6::ChangeAvailabilityReq &req) override {
            std::vector<bool*> inoperative_connectors;

            for (auto const& entry : station_->getConnectorMetadata()) {
                if (req.connectorId == 0 || entry.second.connector_id1_6 == req.connectorId) {
                    inoperative_connectors.emplace_back(&inoperative_connectors_[entry.first]);
                }
            }

            if (inoperative_connectors.empty())
                return ocpp1_6::ChangeAvailabilityRsp {ocpp1_6::AvailabilityStatus::kRejected};

            switch (req.type) {
                default:
                    return ocpp1_6::CallError {
                            ocpp1_6::ErrorCode::kPropertyConstraintViolation,
                            "Bad ChangeAvailabilityReq type enum",
                            common::RawJson::empty_object()
                    };

                case ocpp1_6::AvailabilityType::kOperative:
                    for (auto &connector : inoperative_connectors) {
                        *connector = false;
                    }
                    break;

                case ocpp1_6::AvailabilityType::kInoperative:
                    for (auto &connector : inoperative_connectors) {
                        *connector = true;
                    }
                    break;
            }

            bool charging = false;
            for (auto const& entry : station_->getConnectorMetadata()) {
                if (req.connectorId != 0) {
                    if (req.connectorId != entry.second.connector_id1_6)
                        continue;
                }

                auto const status = station_->pollConnectorStatus(entry.first);
                if (status.has_value() && status->charging_enabled) {
                    charging = true;
                    break;
                }
            }

            saveInoperativeConnectors();

            if (charging) {
                return ocpp1_6::ChangeAvailabilityRsp {ocpp1_6::AvailabilityStatus::kScheduled};
            } else {
                return ocpp1_6::ChangeAvailabilityRsp {ocpp1_6::AvailabilityStatus::kAccepted};
            }
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::ChangeAvailabilityResponse>>
        onChangeAvailabilityReq(const ocpp2_0::ChangeAvailabilityRequest &req) override {
            std::map<ocpp2_0::EVSEType, charger::ConnectorMetadata> metadata;
            for (auto const& entry : station_->getConnectorMetadata()) {
                if (req.evse.has_value()) {
                    if (req.evse->id != entry.first.id)
                        continue;
                    if (req.evse->connectorId.has_value() && req.evse->connectorId != entry.first.connectorId)
                        continue;
                }

                metadata[entry.first] = entry.second;
            }

            if (metadata.size() == 0)
                return ocpp2_0::ChangeAvailabilityResponse {ocpp2_0::ChangeAvailabilityStatusEnumType::kRejected};

            switch (req.operationalStatus) {
                default:
                    return ocpp2_0::CallError {
                            ocpp2_0::ErrorCode::kPropertyConstraintViolation,
                            "Bad ChangeAvailabilityRequest operationalStatus enum",
                            common::RawJson::empty_object()
                    };

                case ocpp2_0::OperationalStatusEnumType::kOperative:
                    inoperative_connectors_[req.evse] = false;
                    break;

                case ocpp2_0::OperationalStatusEnumType::kInoperative:
                    inoperative_connectors_[req.evse] = true;
                    break;
            }

            bool charging = false;
            for (auto const& entry : metadata) {
                auto const status = station_->pollConnectorStatus(entry.first);
                if (status.has_value() && status->charging_enabled) {
                    charging = true;
                    break;
                }
            }

            saveInoperativeConnectors();

            if (charging) {
                return ocpp2_0::ChangeAvailabilityResponse {ocpp2_0::ChangeAvailabilityStatusEnumType::kScheduled};
            } else {
                return ocpp2_0::ChangeAvailabilityResponse {ocpp2_0::ChangeAvailabilityStatusEnumType::kAccepted};
            }
        }

    private:
        void advanceConnectorState(detail::ReportedConnectorStatus& reported, charger::ConnectorStatus const& current) {
            if (current.vehicle_connected && current.charging_enabled) {
                reported.current_plug_was_charging = true;
            } else if (!current.vehicle_connected) {
                reported.current_plug_was_charging = false;
            }
        }

        void saveInoperativeConnectors() {
            std::string result;
            char const* ifs = "";
            for (auto const& x : inoperative_connectors_) {
                if (!x.second)
                    continue;

                result += ifs;
                if (!x.first.has_value()) {
                    result += "0";
                } else if (!x.first->connectorId.has_value()) {
                    result += std::to_string(x.first->id);
                } else {
                    result += std::to_string(x.first->id) + "." + std::to_string(x.first->connectorId.value());
                }
                ifs = ",";
            }

            settings_->InoperativeConnectors.setValue(result);
            CHARGELAB_LOG_MESSAGE(debug) << "Saved inoperative connectors: " << result;
        }

        void loadInoperativeConnectors() {
            auto const value = settings_->InoperativeConnectors.getValue();
            CHARGELAB_LOG_MESSAGE(debug) << "Loading inoperative connectors: " << value;

            inoperative_connectors_.clear();
            string::SplitVisitor(value, ",", [&](std::string const& text) {
                if (text.empty())
                    return;

                auto const it = text.find('.');
                if (it != std::string::npos) {
                    auto const id = string::ToInteger(text.substr(0, it));
                    auto const connectorId = string::ToInteger(text.substr(it+1));
                    if (id && connectorId) {
                        inoperative_connectors_[ocpp2_0::EVSEType{id.value(), connectorId}] = true;
                    }
                } else {
                    auto const id = string::ToInteger(text);
                    if (id) {
                        if (id.value() == 0) {
                            inoperative_connectors_[std::nullopt] = true;
                        } else {
                            inoperative_connectors_[ocpp2_0::EVSEType{id.value()}] = true;
                        }
                    }
                }
            });

            CHARGELAB_LOG_MESSAGE(debug) << "Inoperative connectors: " << inoperative_connectors_;
        }

        ocpp2_0::ConnectorStatusEnumType getStatus2_0(ocpp2_0::EVSEType const& evse, charger::ConnectorStatus const& status) {
            if (status.faulted_status.has_value())
                return ocpp2_0::ConnectorStatusEnumType::kFaulted;
            if (status.vehicle_connected && status.charging_enabled)
                return ocpp2_0::ConnectorStatusEnumType::kOccupied;

            // TODO: Need to explore this further. This is failing TC_B_22_CS in OCTT; is that because the transaction
            //  stop message wasn't received first or because the station must not send out an Unavailable status under
            //  these conditions?
//            if (pending_reset_)
//                return ocpp2_0::ConnectorStatusEnumType::kUnavailable;

            if (!status.connector_available)
                return ocpp2_0::ConnectorStatusEnumType::kUnavailable;
            if (status.vehicle_connected)
                return ocpp2_0::ConnectorStatusEnumType::kOccupied;

            // TODO: What is the precedence of "Unavailable" in 2.0.1?
            // Note: this fails test case TC_G_11_CS if this takes precedence over the "Occupied" state above.
            if (inoperative_connectors_[std::nullopt])
                return ocpp2_0::ConnectorStatusEnumType::kUnavailable;
            if (inoperative_connectors_[ocpp2_0::EVSEType {evse.id}])
                return ocpp2_0::ConnectorStatusEnumType::kUnavailable;
            if (inoperative_connectors_[evse])
                return ocpp2_0::ConnectorStatusEnumType::kUnavailable;

            return ocpp2_0::ConnectorStatusEnumType::kAvailable;
        }

        ocpp1_6::ChargePointStatus getStatus1_6(ocpp2_0::EVSEType const& evse, charger::ConnectorStatus const& current, bool was_charging) {
            if (current.faulted_status.has_value())
                return ocpp1_6::ChargePointStatus::kFaulted;

            if (current.vehicle_connected && current.charging_enabled) {
                if (current.suspended_by_charger) {
                    return ocpp1_6::ChargePointStatus::kSuspendedEVSE;
                } else if (current.suspended_by_vehicle) {
                    return ocpp1_6::ChargePointStatus::kSuspendedEV;
                } else {
                    return ocpp1_6::ChargePointStatus::kCharging;
                }
            }

            if (inoperative_connectors_[std::nullopt])
                return ocpp1_6::ChargePointStatus::kUnavailable;
            if (inoperative_connectors_[evse])
                return ocpp1_6::ChargePointStatus::kUnavailable;

//            if (pending_reset_)
//                return ocpp1_6::ChargePointStatus::kUnavailable;

            if (!current.connector_available)
                return ocpp1_6::ChargePointStatus::kUnavailable;
            if (current.vehicle_connected) {
                if (was_charging) {
                    return ocpp1_6::ChargePointStatus::kFinishing;
                } else {
                    return ocpp1_6::ChargePointStatus::kPreparing;
                }
            }

            // For RFID, we have a pending start entry but with a nullopt value
            auto it = std::find_if(pending_start_.begin(), pending_start_.end(), [=] (auto entry) {
                return !entry.has_value() || entry->id == evse.id; } );

            // Connector 0 can't be preparing
            if (it != pending_start_.end() && evse.id != 0) {
                return ocpp1_6::ChargePointStatus::kPreparing;
            }

            return ocpp1_6::ChargePointStatus::kAvailable;
        }

        template <typename Map, typename Key, typename Generator>
        auto& getOrCreateSetting(Map& map, Key&& key, Generator&& generator) {
            auto it = map.find(key);
            if (it != map.end() && it->second != nullptr)
                return *it->second;

            auto setting = generator();
            map[key] = setting;
            settings_->registerCustomSetting(setting);
            return *setting;
        }

        void addAndUpdateStateSettings() {
            auto const now = platform_->steadyClockNow();
            if (last_settings_update_.has_value()) {
                auto const delta = now - last_settings_update_.value();
                if (delta < kSettingUpdateIntervalMillis)
                    return;
            }
            last_settings_update_ = now;

            // Update station level settings
            {
                auto const& metadata = station_->getStationMetadata();
                settings_->ChargingStationAvailable.setValue(true);
                settings_->ChargingStationAvailabilityState.setValue(
                        inoperative_connectors_[std::nullopt] ? "Unavailable" : "Available");
                settings_->ChargingStationSupplyPhases.setValue(metadata.supply_phases);
            }

            // Update connector level settings
            std::set<int> evse_ids {};
            for (auto const& entry : station_->getConnectorMetadata()) {
                if (!entry.first.connectorId.has_value()) {
                    CHARGELAB_LOG_MESSAGE(error) << "Bad connector metadata - must specify a connector ID";
                    continue;
                }

                auto const& evse_id = entry.first.id;
                auto const& connector_id = entry.first.connectorId.value();
                auto const& connector_status = station_->pollConnectorStatus(entry.first);
                evse_ids.insert(evse_id);

                // 2.13.1
                auto& available = getOrCreateSetting(settings_available_, entry.first, [&]() {
                    return std::make_shared<SettingBool>(
                            std::make_unique<SettingMetadata>(SettingMetadata {
                                    "EVSE" + std::to_string(evse_id) + "Connector" + std::to_string(connector_id) + "Available",
                                    SettingConfig::roNotSavedPolicy(),
                                    std::nullopt,
                                    DeviceModel2_0{
                                            ocpp2_0::ComponentType{"Connector", std::nullopt, ocpp2_0::EVSEType{evse_id, connector_id}},
                                            ocpp2_0::VariableType{"Available"},
                                            ocpp2_0::VariableCharacteristicsType{std::nullopt, ocpp2_0::DataEnumType::kboolean}
                                    },
                                    SettingBool::kTextTrue
                            }),
                            [](auto const&) {return true;}
                    );
                });
                available.setValue(connector_status.has_value());

                // 2.13.2
                auto& availability_state = getOrCreateSetting(settings_availability_state_, entry.first, [&]() {
                    return std::make_shared<SettingString>(
                            std::make_unique<SettingMetadata>(SettingMetadata {
                                    "EVSE" + std::to_string(evse_id) + "Connector" + std::to_string(connector_id) + "AvailabilityState",
                                    SettingConfig::roNotSavedPolicy(),
                                    std::nullopt,
                                    DeviceModel2_0{
                                            ocpp2_0::ComponentType{"Connector", std::nullopt, ocpp2_0::EVSEType{evse_id, connector_id}},
                                            ocpp2_0::VariableType{"AvailabilityState"},
                                            ocpp2_0::VariableCharacteristicsType{
                                                std::nullopt,
                                                ocpp2_0::DataEnumType::kOptionList,
                                                std::nullopt,
                                                std::nullopt,
                                                "Available,Occupied,Reserved,Unavailable,Faulted"
                                            }
                                    },
                                    "Unavailable"
                            }),
                            [](auto const&) {return true;}
                    );
                });

                if (connector_status.has_value()) {
                    auto const& status = getStatus2_0(entry.first, connector_status.value());
                    availability_state.setValue(status.to_string());
                }

                // 2.13.4
                auto& connector_type = getOrCreateSetting(settings_connector_type_, entry.first, [&]() {
                    auto const& name = "EVSE" + std::to_string(evse_id) + "Connector" + std::to_string(connector_id) + "ConnectorType";
                    return std::make_shared<SettingString>(
                            std::make_unique<SettingMetadata>(SettingMetadata {
                                    name,
                                    SettingConfig::roNotSavedPolicy(),
                                    DeviceModel1_6{name},
                                    DeviceModel2_0{
                                            ocpp2_0::ComponentType{"Connector", std::nullopt, ocpp2_0::EVSEType{evse_id, connector_id}},
                                            ocpp2_0::VariableType{"ConnectorType"},
                                            ocpp2_0::VariableCharacteristicsType{std::nullopt, ocpp2_0::DataEnumType::kstring}
                                    },
                                    ""
                            }),
                            [](auto const&) {return true;}
                    );
                });
                connector_type.setValue(entry.second.connector_type);

                // 2.13.6
                auto& supply_phases = getOrCreateSetting(settings_supply_phases_, entry.first, [&]() {
                    auto const& name = "EVSE" + std::to_string(evse_id) + "Connector" + std::to_string(connector_id) + "SupplyPhases";
                    return std::make_shared<SettingInt>(
                            std::make_unique<SettingMetadata>(SettingMetadata {
                                    name,
                                    SettingConfig::roNotSavedPolicy(),
                                    DeviceModel1_6{name},
                                    DeviceModel2_0{
                                            ocpp2_0::ComponentType{"Connector", std::nullopt, ocpp2_0::EVSEType{evse_id, connector_id}},
                                            ocpp2_0::VariableType{"SupplyPhases"},
                                            ocpp2_0::VariableCharacteristicsType{std::nullopt, ocpp2_0::DataEnumType::kinteger}
                                    },
                                    std::to_string(1)
                            }),
                            [](auto const&) {return true;}
                    );
                });
                supply_phases.setValue(entry.second.supply_phases);

                // 2.13.7
                // Note: max power won't be updated dynamically if the station information changes; this could possibly
                // be improved in the future, but that may not be something that's ever needed in practice.
                auto& power = getOrCreateSetting(settings_power_, entry.first, [&]() {
                    auto const& name = "EVSE" + std::to_string(evse_id) + "Connector" + std::to_string(connector_id) + "Power";
                    return std::make_shared<SettingDouble>(
                            std::make_unique<SettingMetadata>(SettingMetadata {
                                    name,
                                    SettingConfig::roNotSavedPolicy(),
                                    DeviceModel1_6{name},
                                    DeviceModel2_0{
                                            ocpp2_0::ComponentType{"Connector", std::nullopt, ocpp2_0::EVSEType{evse_id, connector_id}},
                                            ocpp2_0::VariableType{"Power"},
                                            ocpp2_0::VariableCharacteristicsType{
                                                "W",
                                                ocpp2_0::DataEnumType::kdecimal,
                                                std::nullopt,
                                                entry.second.power_max_watts
                                            }
                                    },
                                    std::to_string(0)
                            }),
                            [](auto const&) {return true;}
                    );
                });

                // TODO: Take this from the supplied meter values for the connector? Note that a wattage reading isn't
                //  mandatory, in which case we'd need to support not reporting the actual characteristic.
                power.setValue(0);
            }

            // Update EVSE level settings
            for (auto const& entry : station_->getEvseMetadata()) {
                if (entry.first.connectorId.has_value()) {
                    CHARGELAB_LOG_MESSAGE(error) << "Bad EVSE metadata - should not specify a connector ID";
                    continue;
                }

                // 2.13.1
                getOrCreateSetting(settings_available_, ocpp2_0::EVSEType{entry.first.id}, [&]() {
                    return std::make_shared<SettingBool>(
                            std::make_unique<SettingMetadata>(SettingMetadata {
                                    "EVSE" + std::to_string(entry.first.id) + "Available",
                                    SettingConfig::roNotSavedPolicy(),
                                    std::nullopt,
                                    DeviceModel2_0{
                                            ocpp2_0::ComponentType{"EVSE", std::nullopt, ocpp2_0::EVSEType{entry.first.id}},
                                            ocpp2_0::VariableType{"Available"},
                                            ocpp2_0::VariableCharacteristicsType{std::nullopt, ocpp2_0::DataEnumType::kboolean}
                                    },
                                    SettingBool::kTextTrue
                            }),
                            [](auto const&) {return true;}
                    );
                });

                // 2.13.2
                auto& availability_state = getOrCreateSetting(settings_availability_state_, ocpp2_0::EVSEType{entry.first.id}, [&]() {
                    return std::make_shared<SettingString>(
                            std::make_unique<SettingMetadata>(SettingMetadata {
                                    "EVSE" + std::to_string(entry.first.id) + "AvailabilityState",
                                    SettingConfig::roNotSavedPolicy(),
                                    std::nullopt,
                                    DeviceModel2_0{
                                            ocpp2_0::ComponentType{"EVSE", std::nullopt, ocpp2_0::EVSEType{entry.first.id}},
                                            ocpp2_0::VariableType{"AvailabilityState"},
                                            ocpp2_0::VariableCharacteristicsType{
                                                    std::nullopt,
                                                    ocpp2_0::DataEnumType::kOptionList,
                                                    std::nullopt,
                                                    std::nullopt,
                                                    "Available,Occupied,Reserved,Unavailable,Faulted"
                                            }
                                    },
                                    "Unavailable"
                            }),
                            [](auto const&) {return true;}
                    );
                });

                // TODO: How should this be interpreted when there's more than one connector under an EVSE? Is it
                //  reasonable to only report "Available"/"Unavailable" here based on what was configured via
                //  ChangeAvailability?
                availability_state.setValue(inoperative_connectors_[entry.first] ? "Unavailable" : "Available");

                // 2.13.6
                auto& supply_phases = getOrCreateSetting(settings_supply_phases_, entry.first, [&]() {
                    auto const& name = "EVSE" + std::to_string(entry.first.id) + "SupplyPhases";
                    return std::make_shared<SettingInt>(
                            std::make_unique<SettingMetadata>(SettingMetadata {
                                    name,
                                    SettingConfig::roNotSavedPolicy(),
                                    DeviceModel1_6{name},
                                    DeviceModel2_0{
                                            ocpp2_0::ComponentType{"EVSE", std::nullopt, ocpp2_0::EVSEType{entry.first.id}},
                                            ocpp2_0::VariableType{"SupplyPhases"},
                                            ocpp2_0::VariableCharacteristicsType{std::nullopt, ocpp2_0::DataEnumType::kinteger}
                                    },
                                    std::to_string(1)
                            }),
                            [](auto const&) {return true;}
                    );
                });
                supply_phases.setValue(entry.second.supply_phases);

                // 2.13.7
                // Note: max power won't be updated dynamically if the station information changes; this could possibly
                // be improved in the future, but that may not be something that's ever needed in practice.
                auto& power = getOrCreateSetting(settings_power_, entry.first, [&]() {
                    auto const& name = "EVSE" + std::to_string(entry.first.id) + "Power";
                    return std::make_shared<SettingDouble>(
                            std::make_unique<SettingMetadata>(SettingMetadata {
                                    name,
                                    SettingConfig::roNotSavedPolicy(),
                                    DeviceModel1_6{name},
                                    DeviceModel2_0{
                                            ocpp2_0::ComponentType{"EVSE", std::nullopt, ocpp2_0::EVSEType{entry.first.id}},
                                            ocpp2_0::VariableType{"Power"},
                                            ocpp2_0::VariableCharacteristicsType{
                                                    "W",
                                                    ocpp2_0::DataEnumType::kdecimal,
                                                    std::nullopt,
                                                    entry.second.power_max_watts
                                            }
                                    },
                                    std::to_string(0)
                            }),
                            [](auto const&) {return true;}
                    );
                });

                // TODO: Take this from the supplied meter values for the connector? Note that a wattage reading isn't
                //  mandatory, in which case we'd need to support not reporting the actual characteristic.
                power.setValue(0);
            }
        }

    private:
        std::shared_ptr<Settings> settings_;
        std::shared_ptr<PlatformInterface> platform_;
        std::shared_ptr<StationInterface> station_;

        OperationHolder<std::string> pending_connector_status_req_;
        std::optional<std::pair<chargelab::ocpp2_0::EVSEType, ocpp2_0::StatusNotificationRequest>> pending_connector_status_update2_0_ = std::nullopt;
        std::optional<std::pair<chargelab::ocpp2_0::EVSEType, ocpp1_6::StatusNotificationReq>> pending_connector_status_update1_6_ = std::nullopt;
        std::map<chargelab::ocpp2_0::EVSEType, detail::ReportedConnectorStatus> reported_status_;
        std::map<std::optional<chargelab::ocpp2_0::EVSEType>, bool> inoperative_connectors_;
        std::atomic<bool> pending_reset_ = false;
        std::atomic<bool> reset_reason_hard_ = false;   // hard reset or soft reset
        std::optional<SteadyPointMillis> last_online_ = std::nullopt;

        std::vector<std::optional<ocpp2_0::EVSEType>> pending_start_;

        std::optional<SteadyPointMillis> last_settings_update_ = std::nullopt;
        std::map<chargelab::ocpp2_0::EVSEType, std::shared_ptr<SettingBool>> settings_available_ {};
        std::map<chargelab::ocpp2_0::EVSEType, std::shared_ptr<SettingString>> settings_availability_state_ {};
        std::map<chargelab::ocpp2_0::EVSEType, std::shared_ptr<SettingString>> settings_connector_type_ {};
        std::map<chargelab::ocpp2_0::EVSEType, std::shared_ptr<SettingInt>> settings_supply_phases_ {};
        std::map<chargelab::ocpp2_0::EVSEType, std::shared_ptr<SettingDouble>> settings_power_ {};

        // For portal module
        std::unordered_map<int, ocpp1_6::ChargePointStatus> last_charge_point_status_map1_6_ {};
        std::mutex last_charge_point_status_map1_6_mutex_;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_CONNECTOR_STATUS_MODULE_H
