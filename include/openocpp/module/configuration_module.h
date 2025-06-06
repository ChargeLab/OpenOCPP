#ifndef CHARGELAB_OPEN_FIRMWARE_CONFIGURATION_MODULE_H
#define CHARGELAB_OPEN_FIRMWARE_CONFIGURATION_MODULE_H

#include "openocpp/protocol/common/protocol_constants.h"
#include "openocpp/module/common_templates.h"
#include "openocpp/common/settings.h"
#include "openocpp/helpers/string.h"
#include "openocpp/interface/component/system_interface.h"

#include <utility>
#include <functional>

namespace chargelab {
    class ConfigurationModule : public ServiceStatefulGeneral {
    private:
        static constexpr char const* kMaskedValue = "****";
        static constexpr int kOcpp20NotifyReportRequestOverheadBytes = 100;

    public:
        explicit ConfigurationModule(std::shared_ptr<Settings> settings, std::shared_ptr<SystemInterface> system)
            : settings_(std::move(settings)),
              system_(std::move(system))
        {
            assert(settings_ != nullptr);
            assert(system_ != nullptr);
        }

        ~ConfigurationModule() override {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting ConfigurationModule";
            settings_->saveIfModified();
        }

    public:
        void runUnconditionally() override {
            settings_->saveIfModified();
        }

        void runStep(ocpp1_6::OcppRemote&) override {
        }

        void runStep(ocpp2_0::OcppRemote &remote) override {
            if (ocpp2_0_pending_base_report_.has_value()) {
                auto request = ocpp2_0_pending_base_report_.value();
                auto settings = settings_;
                ocpp2_0::NotifyReportRequest response {
                        request.requestId,
                        {system_->systemClockNow()},
                        false,
                        0,
                        {[&](std::function<void(ocpp2_0::ReportDataType const &)> const &visitor) {
                            settings->visitSettings([&](SettingBase& setting) {
                                auto const metadata = setting.getMetadata();
                                bool include_characteristics;
                                switch (request.reportBase) {
                                    default:
                                        CHARGELAB_LOG_MESSAGE(error) << "Unexpected report type in generator: " << request.reportBase;
                                        return;

                                    case ocpp2_0::ReportBaseEnumType::kConfigurationInventory:
                                        include_characteristics = true;
                                        if (!metadata.config.isAllowOcppWrite())
                                            return;
                                        break;

                                    case ocpp2_0::ReportBaseEnumType::kFullInventory:
                                        include_characteristics = true;
                                        break;
                                }

                                if (!metadata.model2_0.has_value())
                                    return;

                                auto element = ocpp2_0::ReportDataType {
                                        metadata.model2_0->component_type,
                                        metadata.model2_0->variable_type,
                                        setting.getAttributes2_0()
                                };

                                if (include_characteristics) {
                                    element.variableCharacteristics = metadata.model2_0->variable_characteristics;
                                }

                                visitor(element);
                            });
                        }}
                };

                if (remote.sendNotifyReportReq(response).has_value()) {
                    ocpp2_0_pending_base_report_ = std::nullopt;
                }
            }
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetVariablesResponse>>
        onGetVariablesReq(const ocpp2_0::GetVariablesRequest &request) override {
            if ((int)request.getVariableData.size() > settings_->ItemsPerMessageGetVariables.getValue()) {
                return ocpp2_0::CallError {
                    ocpp2_0::ErrorCode::kOccurrenceConstraintViolation,
                    {"Exceeded ItemsPerMessageGetVariables limit"},
                    common::RawJson::empty_object()
                };
            }

            auto settings = settings_;
            return ocpp2_0::GetVariablesResponse {
                    {[=](std::function<void(ocpp2_0::GetVariableResultType const &)> const &visitor) {
                        for (auto const& get_variable : request.getVariableData) {
                            bool componentFound = false;
                            bool variableFound = false;
                            auto result = ocpp2_0::GetVariableResultType {
                                    ocpp2_0::GetVariableStatusEnumType::kValueNotFoundInEnum,
                                    get_variable.attributeType,
                                    std::nullopt,
                                    get_variable.component,
                                    get_variable.variable
                            };

                            settings->visitSettings([&](SettingBase& setting) {
                                auto const metadata = setting.getMetadata();
                                if (variableFound)
                                    return;
                                if (!metadata.model2_0.has_value())
                                    return;

                                if (metadata.model2_0->component_type != get_variable.component) {
                                    return;
                                } else {
                                    componentFound = true;
                                }

                                if (metadata.model2_0->variable_type != get_variable.variable) {
                                    return;
                                } else {
                                    variableFound = true;
                                }

                                if (get_variable.attributeType.has_value()) {
                                    // TODO - right now everything defined is "actual"
                                    if (get_variable.attributeType.value() != ocpp2_0::AttributeEnumType::kActual) {
                                        result.attributeStatus = ocpp2_0::GetVariableStatusEnumType::kNotSupportedAttributeType;
                                        return;
                                    }
                                }

                                if (!metadata.config.isAllowOcppRead()) {
                                    result.attributeStatus = ocpp2_0::GetVariableStatusEnumType::kRejected;
                                    return;
                                }

                                result.attributeStatus = ocpp2_0::GetVariableStatusEnumType::kAccepted;
                                result.attributeValue = setting.getValueAsString();
                            });

                            if (!componentFound) {
                                result.attributeStatus = ocpp2_0::GetVariableStatusEnumType::kUnknownComponent;
                            } else if (!variableFound) {
                                result.attributeStatus = ocpp2_0::GetVariableStatusEnumType::kUnknownVariable;
                            }

                            visitor(result);
                        }
                    }}
            };
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::SetVariablesResponse>>
        onSetVariablesReq(const ocpp2_0::SetVariablesRequest &request) override {
            std::vector<ocpp2_0::SetVariableResultType> results;
            for (auto const& set_variable : request.setVariableData) {
                bool componentFound = false;
                bool variableFound = false;
                auto result = ocpp2_0::SetVariableResultType {
                        set_variable.attributeType,
                        ocpp2_0::SetVariableStatusEnumType::kValueNotFoundInEnum,
                        set_variable.component,
                        set_variable.variable
                };

                auto const network_configuration_priority_id = settings_->NetworkConfigurationPriority.getId();
                settings_->visitSettings([&](SettingBase& setting) {
                    auto const metadata = setting.getMetadata();
                    if (variableFound)
                        return;
                    if (!metadata.model2_0.has_value())
                        return;

                    if (metadata.model2_0->component_type != set_variable.component) {
                        return;
                    } else {
                        componentFound = true;
                    }

                    if (metadata.model2_0->variable_type != set_variable.variable) {
                        return;
                    } else {
                        variableFound = true;
                    }

                    if (set_variable.attributeType.has_value()) {
                        // TODO - right now everything defined is "actual"
                        if (set_variable.attributeType.value() != ocpp2_0::AttributeEnumType::kActual) {
                            result.attributeStatus = ocpp2_0::SetVariableStatusEnumType::kNotSupportedAttributeType;
                            return;
                        }
                    }

                    if (!metadata.config.isAllowOcppWrite()) {
                        result.attributeStatus = ocpp2_0::SetVariableStatusEnumType::kRejected;
                        return;
                    }

                    // A05.FR.02
                    if (metadata.id == network_configuration_priority_id) {
                        bool missing_profile = false;
                        int target_security_profile = 0;
                        string::SplitVisitor(set_variable.attributeValue.value(), ",", [&](std::string const& text) {
                            auto slot = string::ToInteger(text);
                            if (!slot.has_value())
                                return;

                            auto const& profile = settings_->NetworkConnectionProfiles.getValue(slot.value());
                            if (!profile.has_value()) {
                                missing_profile = true;
                                return;
                            }

                            target_security_profile = std::max(target_security_profile, profile->securityProfile);
                        });

                        // TODO: Is there a specific requirement for this?
                        if (target_security_profile < settings_->SecurityProfile.getValue()) {
                            result.attributeStatus = ocpp2_0::SetVariableStatusEnumType::kRejected;
                            return;
                        }

                        if (target_security_profile > settings_->SecurityProfile.getValue()) {
                            // A05.FR.02
                            if (target_security_profile >= 2 && settings_->InstalledCSMSRootCertificateCount.getValue() <= 0) {
                                result.attributeStatus = ocpp2_0::SetVariableStatusEnumType::kRejected;
                                return;
                            }

                            // A05.FR.03
                            // TODO: Not relevant until we have something that can store a charging station certificate
                            if (target_security_profile == 3) {
                                result.attributeStatus = ocpp2_0::SetVariableStatusEnumType::kRejected;
                                return;
                            }
                        }
                    }

                    // TODO: move to update attribute?
                    if (!settings_->setSettingValue(metadata.id, set_variable.attributeValue.value())) {
                        result.attributeStatus = ocpp2_0::SetVariableStatusEnumType::kRejected;
                        return;
                    }

                    if (metadata.config.isRebootRequired()) {
                        result.attributeStatus = ocpp2_0::SetVariableStatusEnumType::kRebootRequired;
                    } else {
                        result.attributeStatus = ocpp2_0::SetVariableStatusEnumType::kAccepted;
                    }
                });

                if (!componentFound) {
                    result.attributeStatus = ocpp2_0::SetVariableStatusEnumType::kUnknownComponent;
                } else if (!variableFound) {
                    result.attributeStatus = ocpp2_0::SetVariableStatusEnumType::kUnknownVariable;
                }

                results.push_back(std::move(result));
            }
            
            return ocpp2_0::SetVariablesResponse {std::move(results)};
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetBaseReportResponse>>
        onGetBaseReportReq(const ocpp2_0::GetBaseReportRequest &request) override {
            if (ocpp2_0_pending_base_report_.has_value()) {
                return ocpp2_0::GetBaseReportResponse {
                        ocpp2_0::GenericDeviceModelStatusEnumType::kRejected
                };
            }

            switch (request.reportBase) {
                case ocpp2_0::ReportBaseEnumType::kValueNotFoundInEnum:
                case ocpp2_0::ReportBaseEnumType::kSummaryInventory:
                    return ocpp2_0::GetBaseReportResponse {
                            ocpp2_0::GenericDeviceModelStatusEnumType::kNotSupported
                    };

                case ocpp2_0::ReportBaseEnumType::kFullInventory:
                case ocpp2_0::ReportBaseEnumType::kConfigurationInventory:
                    break;
            }

            ocpp2_0_pending_base_report_ = request;
            return ocpp2_0::GetBaseReportResponse {
                    ocpp2_0::GenericDeviceModelStatusEnumType::kAccepted
            };
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ChangeConfigurationRsp>>
        onChangeConfigurationReq(const ocpp1_6::ChangeConfigurationReq& req) override {
            return changeConfiguration(req, false);
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ChangeConfigurationRsp>>
        changeConfiguration(const ocpp1_6::ChangeConfigurationReq& req, bool force_change) {
            CHARGELAB_LOG_MESSAGE(info) << "changing configuration, key:" << req.key.value() << ", value:" << req.value.value();
            auto const state = settings_->getSettingState(req.key.value());
            if (!force_change) {
                if (!state.has_value() || (!state->config.isAllowOcppWrite() && !state->config.isAllowOcppRead()))
                    return ocpp1_6::ChangeConfigurationRsp{ocpp1_6::ConfigurationStatus::kNotSupported};
                if (!state->config.isAllowOcppWrite())
                    return ocpp1_6::ChangeConfigurationRsp{ocpp1_6::ConfigurationStatus::kRejected};
            }

            // TODO: Move to 1_6 device model rather than id
            if (!settings_->setSettingValue(req.key.value(), req.value.value()))
                return ocpp1_6::ChangeConfigurationRsp{ocpp1_6::ConfigurationStatus::kRejected};

            if (force_change) {
                settings_->saveIfModified();
            }

            if (state->config.isRebootRequired()) {
                return ocpp1_6::ChangeConfigurationRsp{ocpp1_6::ConfigurationStatus::kRebootRequired};
            } else {
                return ocpp1_6::ChangeConfigurationRsp{ocpp1_6::ConfigurationStatus::kAccepted};
            }
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::GetConfigurationRsp>>
        onGetConfigurationReq(const ocpp1_6::GetConfigurationReq &req) override {
            return getConfiguration(req);
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::GetConfigurationRsp>>
        getConfiguration(const ocpp1_6::GetConfigurationReq &req) {
            std::vector<std::string> include_keys;
            std::vector<ocpp1_6::CiString50Type> unknown_keys = req.key.value();

            // TODO: Move to 1_6 device model rather than id
            bool include_all_keys = req.key.value().empty();
            if (!unknown_keys.empty()) {
                if ((int)req.key.value().size() > settings_->GetConfigurationMaxKeys.getValue()) {
                    return ocpp1_6::CallError {
                            ocpp1_6::ErrorCode::kPropertyConstraintViolation,
                            "ConnectorId is not same as the local connector Id - must be same",
                            common::RawJson::empty_object()
                    };
                }

                unknown_keys = req.key.value();
                settings_->visitSettings([&](SettingBase const& setting) {
                    auto const metadata = setting.getMetadata();
                    if (!metadata.config.isAllowOcppRead() && !metadata.config.isAllowOcppWrite())
                        return;

                    auto it = std::remove_if(unknown_keys.begin(), unknown_keys.end(), [&] (auto const& x) {
                        return string::EqualsIgnoreCaseAscii(x.value(), metadata.id);
                    });

                    if (it != unknown_keys.end()) {
                        include_keys.push_back(metadata.id);
                        unknown_keys.erase(it, unknown_keys.end());
                    }
                });
            }

            auto settings = settings_;
            return ocpp1_6::GetConfigurationRsp {
                    {[=](std::function<void(ocpp1_6::KeyValue const &)> const &visitor) {
                        auto const& keys = include_keys;
                        settings->visitSettings([&](SettingBase const& setting) {
                            auto const metadata = setting.getMetadata();
                            if (!metadata.config.isAllowOcppRead() && !metadata.config.isAllowOcppWrite())
                                return;

                            std::string value;
                            if (metadata.config.isAllowOcppRead()) {
                                value = setting.getValueAsString();
                            } else {
                                value = kMaskedValue;
                            }

                            if (include_all_keys || std::find(keys.begin(), keys.end(), setting.getId()) != keys.end()) {
                                visitor(ocpp1_6::KeyValue{
                                        {metadata.id},
                                        !metadata.config.isAllowOcppWrite(),
                                        std::move(value)
                                });
                            }
                        });
                    }},
                    std::move(unknown_keys)
            };
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::SetNetworkProfileResponse>>
        onSetNetworkProfileReq(const ocpp2_0::SetNetworkProfileRequest &request) override {
            if (request.configurationSlot < 0 || request.configurationSlot >= detail::SettingsConstants::kMaxProfileSlots) {
                // B09.FR.02
                return ocpp2_0::SetNetworkProfileResponse {
                        ocpp2_0::SetNetworkProfileStatusEnumType::kRejected,
                        ocpp2_0::StatusInfoType {{"BadConfigurationSlot"}}
                };
            }

            // Note: SIM/VPN not supported
            auto const& data = request.connectionData;
            if (data.apn.has_value()) {
                // B09.FR.02
                return ocpp2_0::SetNetworkProfileResponse {
                        ocpp2_0::SetNetworkProfileStatusEnumType::kRejected,
                        ocpp2_0::StatusInfoType {{"ApnNotSupported"}}
                };
            }
            if (data.vpn.has_value()) {
                // B09.FR.02
                return ocpp2_0::SetNetworkProfileResponse {
                        ocpp2_0::SetNetworkProfileStatusEnumType::kRejected,
                        ocpp2_0::StatusInfoType {{"VpnNotSupported"}}
                };
            }

            // Note: only JSON is supported
             if (data.ocppTransport != ocpp2_0::OCPPTransportEnumType::kJSON) {
                 // B09.FR.02
                 return ocpp2_0::SetNetworkProfileResponse {
                         ocpp2_0::SetNetworkProfileStatusEnumType::kRejected,
                         ocpp2_0::StatusInfoType {{"BadOcppTransport"}}
                 };
             }

             std::string protocol;
             switch (data.ocppVersion) {
                 default:
                 case ocpp2_0::OCPPVersionEnumType::kOCPP12:
                 case ocpp2_0::OCPPVersionEnumType::kOCPP15:
                     // B09.FR.02
                     return ocpp2_0::SetNetworkProfileResponse {
                             ocpp2_0::SetNetworkProfileStatusEnumType::kRejected,
                             ocpp2_0::StatusInfoType {{"BadOcppVersion"}}
                     };

                 case ocpp2_0::OCPPVersionEnumType::kOCPP16:
                     protocol = ProtocolConstants::kProtocolOcpp1_6;
                     break;

                 case ocpp2_0::OCPPVersionEnumType::kOCPP20:
                     protocol = ProtocolConstants::kProtocolOcpp2_0_1;
                     break;
             }

            // Note: an assumed "wifi" interface is always used here
            if (data.ocppInterface != ocpp2_0::OCPPInterfaceEnumType::kWireless0) {
                // B09.FR.02
                return ocpp2_0::SetNetworkProfileResponse {
                        ocpp2_0::SetNetworkProfileStatusEnumType::kRejected,
                        ocpp2_0::StatusInfoType {{"BadOcppInterface"}}
                };
            }

            auto parsed = uri::ParseWebsocketUri(data.ocppCsmsUrl.value());
            if (!parsed.has_value()) {
                // B09.FR.02
                return ocpp2_0::SetNetworkProfileResponse {
                        ocpp2_0::SetNetworkProfileStatusEnumType::kRejected,
                        ocpp2_0::StatusInfoType {{"BadOcppCsmsUrl"}}
                };
            }

            if (request.connectionData.securityProfile < settings_->SecurityProfile.getValue()) {
                // B09.FR.04
                return ocpp2_0::SetNetworkProfileResponse {
                        ocpp2_0::SetNetworkProfileStatusEnumType::kRejected,
                        ocpp2_0::StatusInfoType {{"BadSecurityProfile"}}
                };
            }

            if (settings_->NetworkConnectionProfiles.setValue(request.configurationSlot, request.connectionData)) {
                // B09.FR.01
                return ocpp2_0::SetNetworkProfileResponse {ocpp2_0::SetNetworkProfileStatusEnumType::kAccepted};
            } else {
                // B09.FR.03
                return ocpp2_0::SetNetworkProfileResponse {ocpp2_0::SetNetworkProfileStatusEnumType::kFailed};
            }
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetMonitoringReportResponse>>
        onGetMonitoringReportReq(const ocpp2_0::GetMonitoringReportRequest&) override {
            return ocpp2_0::GetMonitoringReportResponse {ocpp2_0::GenericDeviceModelStatusEnumType::kEmptyResultSet};
        }

    private:
        template<int N>
        static bool ciEquals(ocpp2_0::IdentifierStringPrimitive<N> const& lhs, ocpp2_0::IdentifierStringPrimitive<N> const& rhs) {
            return string::EqualsIgnoreCaseAscii(lhs.value(), rhs.value());
        }

        template<int N>
        static bool ciEquals(std::optional<ocpp2_0::IdentifierStringPrimitive<N>> const& lhs, std::optional<ocpp2_0::IdentifierStringPrimitive<N>> const& rhs) {
            if (lhs.has_value() != rhs.has_value())
                return false;
            if (!lhs.has_value())
                return true;

            return ciEquals(lhs.value(), rhs.value());
        }

    private:
        std::shared_ptr<Settings> settings_;
        std::shared_ptr<SystemInterface> system_;

        std::optional<ocpp2_0::GetBaseReportRequest> ocpp2_0_pending_base_report_ = std::nullopt;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_CONFIGURATION_MODULE_H
