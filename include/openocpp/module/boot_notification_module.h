#ifndef CHARGELAB_OPEN_FIRMWARE_BOOT_NOTIFICATION_MODULE_H
#define CHARGELAB_OPEN_FIRMWARE_BOOT_NOTIFICATION_MODULE_H

#include <utility>

#include "openocpp/module/common_templates.h"
#include "openocpp/common/operation_holder.h"
#include "openocpp/common/settings.h"
#include "openocpp/common/logging.h"

namespace chargelab {
    class BootNotificationModule : public ServiceStatefulGeneral
    {
        /* OCPP 1.6 Section 4.2 on Pending registration statuses:
         *
         * The Central System MAY also return a Pending registration status to indicate that it wants to retrieve or set
         * certain information on the Charge Point before the Central System will accept the Charge Point. If the Central
         * System returns the Pending status, the communication channel SHOULD NOT be closed by either the Charge
         * Point or the Central System. The Central System MAY send request messages to retrieve information from the
         * Charge Point or change its configuration. The Charge Point SHOULD respond to these messages. The Charge
         * Point SHALL NOT send request messages to the Central System unless it has been instructed by the Central
         * System to do so with a TriggerMessage.req request.
         */

    private:
        static constexpr const int kSecurityProfileMigrationResetGracePeriodMillis = 5*1000;

    public:
        BootNotificationModule(
                std::shared_ptr<Settings> settings,
                std::shared_ptr<SystemInterface> const& system_interface
        )
            : settings_(std::move(settings)),
              system_interface_(system_interface),
              pending_boot_notification_req_ {system_interface}
        {
            assert(system_interface_ != nullptr);

            if (settings_->hasPendingTransition(SettingTransitionType::kConnection)) {
                auto timeout = system_interface_->steadyClockNow() + settings_->ConnectionTransitionTimeout.getValue()*1000;
                if (settings_->startTransition(SettingTransitionType::kConnection)) {
                    CHARGELAB_LOG_MESSAGE(info) << "Starting connection transition";
                    connection_transition_timeout_ = static_cast<SteadyPointMillis>(timeout);
                }
            }

            auto const reason_text = settings_->CustomBootReason.getValue();
            if (!reason_text.empty())
                CHARGELAB_LOG_MESSAGE(info) << "Custom boot reason was: " << settings_->CustomBootReason.getValue();
        }

        ~BootNotificationModule() override {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting BootNotificationModule";
        }

    public:
        bool registrationComplete() {
            return registration_complete_;
        }

    private:
        // OCPP 1.6 implementation
        void runStep(ocpp1_6::OcppRemote &remote) override {
            // Clear this flag if present
            settings_->CustomBootReason.setValue("");

            runStepImpl([&]() {
                // TODO: Improved validation/length safety (truncate?)
                ocpp1_6::BootNotificationReq req {};
                req.chargePointVendor = ocpp1_6::CiString20Type(settings_->ChargerVendor.getValue());
                req.chargePointModel = ocpp1_6::CiString20Type(settings_->ChargerModel.getValue());
                req.chargePointSerialNumber = ocpp1_6::CiString25Type(settings_->ChargerSerialNumber.getValue());
                req.firmwareVersion = ocpp1_6::CiString50Type(settings_->ChargerFirmwareVersion.getValue());
                if (auto iccid = settings_->ChargerICCID.getValue(); !iccid.empty())
                    req.iccid = ocpp1_6::CiString20Type(iccid);
                if (auto imsi = settings_->ChargerIMSI.getValue(); !imsi.empty())
                    req.imsi = ocpp1_6::CiString20Type(imsi);
                if (auto meter_serial_number = settings_->ChargerMeterSerialNumber.getValue(); !meter_serial_number.empty())
                    req.meterSerialNumber = ocpp1_6::CiString25Type(meter_serial_number);
                if (auto meter_type = settings_->ChargerMeterType.getValue(); !meter_type.empty())
                    req.meterType = ocpp1_6::CiString25Type(meter_type);

                return remote.sendBootNotificationReq(req);
            });
        }

        void onBootNotificationRsp(
                const std::string &unique_id,
                const ocpp1_6::ResponseMessage<ocpp1_6::BootNotificationRsp> &rsp
        ) override {
            if (pending_boot_notification_req_ == unique_id) {
                pending_boot_notification_req_ = kNoOperation;

                if (std::holds_alternative<ocpp1_6::BootNotificationRsp>(rsp)) {
                    auto const& value = std::get<ocpp1_6::BootNotificationRsp>(rsp);
                    auto const& ts = value.currentTime.getTimestamp();

                    if (ts.has_value()) {
                        system_interface_->setSystemClock(ts.value());
                    } else {
                        CHARGELAB_LOG_MESSAGE(warning) << "Invalid BootNotification response timestamp: "
                            << value.currentTime;
                    }

                    CHARGELAB_LOG_MESSAGE(info) << "BootNotification response was: " << value;
                    switch (value.status) {
                        case ocpp1_6::RegistrationStatus::kAccepted:
                            requested_next_boot_notification_req_ = std::nullopt;
                            settings_->HeartbeatInterval.setValue(value.interval);
                            break;

                        case ocpp1_6::RegistrationStatus::kValueNotFoundInEnum:
                            CHARGELAB_LOG_MESSAGE(error) << "Invalid BootNotification response status for request ID: " << unique_id;
                            [[fallthrough]];

                        case ocpp1_6::RegistrationStatus::kPending:
                        case ocpp1_6::RegistrationStatus::kRejected:
                            requested_next_boot_notification_req_ = static_cast<SteadyPointMillis> (
                                    system_interface_->steadyClockNow() + value.interval*1000
                            );
                            break;
                    }
                    switch (value.status) {
                        case ocpp1_6::RegistrationStatus::kRejected:
                        case ocpp1_6::RegistrationStatus::kValueNotFoundInEnum:
                            registration_complete_ = false;
                            allow_ocpp_calls_ = false;
                            break;

                        case ocpp1_6::RegistrationStatus::kPending:
                            registration_complete_ = false;
                            allow_ocpp_calls_ = true;
                            break;

                        case ocpp1_6::RegistrationStatus::kAccepted:
                            registration_complete_ = true;
                            allow_ocpp_calls_ = true;
                            break;
                    }
                } else {
                    CHARGELAB_LOG_MESSAGE(warning) << "Error response to BootNotification request: " << std::get<ocpp1_6::CallError> (rsp);
                }
            }
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::TriggerMessageRsp>>
        onTriggerMessageReq(const ocpp1_6::TriggerMessageReq &req) override {
            switch (req.requestedMessage) {
                case ocpp1_6::MessageTrigger::kBootNotification:
                    // Note: deviating from the 2.0.1 specification and allowing boot notification trigger messages
                    //       after registration completed in 1.6 here.
                    force_boot_notification_req_ = true;
                    return ocpp1_6::TriggerMessageRsp{ocpp1_6::TriggerMessageStatus::kAccepted};

                default:
                case ocpp1_6::MessageTrigger::kValueNotFoundInEnum:
                case ocpp1_6::MessageTrigger::kDiagnosticsStatusNotification:
                case ocpp1_6::MessageTrigger::kFirmwareStatusNotification:
                case ocpp1_6::MessageTrigger::kHeartbeat:
                case ocpp1_6::MessageTrigger::kMeterValues:
                case ocpp1_6::MessageTrigger::kStatusNotification:
                    if (!registrationComplete()) {
                        return unauthorizedError1_6();
                    } else {
                        return std::nullopt;
                    }
            }
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::RemoteStartTransactionRsp>>
        onRemoteStartTransactionReq(const ocpp1_6::RemoteStartTransactionReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::RemoteStopTransactionRsp>>
        onRemoteStopTransactionReq(const ocpp1_6::RemoteStopTransactionReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::CancelReservationRsp>>
        onCancelReservationReq(const ocpp1_6::CancelReservationReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ChangeAvailabilityRsp>>
        onChangeAvailabilityReq(const ocpp1_6::ChangeAvailabilityReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ChangeConfigurationRsp>>
        onChangeConfigurationReq(const ocpp1_6::ChangeConfigurationReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ClearCacheRsp>>
        onClearCacheReq(const ocpp1_6::ClearCacheReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ClearChargingProfileRsp>>
        onClearChargingProfileReq(const ocpp1_6::ClearChargingProfileReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::DataTransferRsp>>
        onDataTransferReq(const ocpp1_6::DataTransferReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::GetCompositeScheduleRsp>>
        onGetCompositeScheduleReq(const ocpp1_6::GetCompositeScheduleReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::GetConfigurationRsp>>
        onGetConfigurationReq(const ocpp1_6::GetConfigurationReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::GetDiagnosticsRsp>>
        onGetDiagnosticsReq(const ocpp1_6::GetDiagnosticsReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::GetLocalListVersionRsp>>
        onGetLocalListVersionReq(const ocpp1_6::GetLocalListVersionReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ReserveNowRsp>>
        onReserveNowReq(const ocpp1_6::ReserveNowReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ResetRsp>>
        onResetReq(const ocpp1_6::ResetReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::SendLocalListRsp>>
        onSendLocalListReq(const ocpp1_6::SendLocalListReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::SetChargingProfileRsp>>
        onSetChargingProfileReq(const ocpp1_6::SetChargingProfileReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::UnlockConnectorRsp>>
        onUnlockConnectorReq(const ocpp1_6::UnlockConnectorReq&) override {
            return unauthorizedCallHandler1_6();
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::UpdateFirmwareRsp>>
        onUpdateFirmwareReq(const ocpp1_6::UpdateFirmwareReq&) override {
            return unauthorizedCallHandler1_6();
        }

        // OCPP 2.0.1 implementation
        void runStep(ocpp2_0::OcppRemote &remote) override {
            auto reason = ocpp2_0::BootReasonEnumType::kPowerUp;

            auto const reason_text = settings_->CustomBootReason.getValue();
            if (!reason_text.empty()) {
                reason = ocpp2_0::BootReasonEnumType::from_string(reason_text);
            } else if (force_boot_notification_req_) {
                reason = ocpp2_0::BootReasonEnumType::kTriggered;
            }

            runStepImpl([&]() {
                ocpp2_0::BootNotificationRequest req {};
                req.reason = reason;
                req.chargingStation.serialNumber = settings_->ChargerSerialNumber.getValue();
                req.chargingStation.model = settings_->ChargerModel.getValue();
                req.chargingStation.vendorName = settings_->ChargerVendor.getValue();
                req.chargingStation.firmwareVersion = settings_->ChargerFirmwareVersion.getValue();

                auto modem = ocpp2_0::ModemType {};
                if (auto iccid = settings_->ChargerICCID.getValue(); !iccid.empty())
                    modem.iccid = iccid;
                if (auto imsi = settings_->ChargerIMSI.getValue(); !imsi.empty())
                    modem.imsi = imsi;
                if (modem.iccid.has_value() || modem.imsi.has_value())
                    req.chargingStation.modem = std::move(modem);

                return remote.sendBootNotificationReq(req);
            });
        }

        void onBootNotificationRsp(
                const std::string &unique_id,
                const ocpp2_0::ResponseMessage<ocpp2_0::BootNotificationResponse> &rsp
        ) override {
            if (pending_boot_notification_req_ == unique_id) {
                pending_boot_notification_req_ = kNoOperation;
                settings_->CustomBootReason.setValue("");

                if (std::holds_alternative<ocpp2_0::BootNotificationResponse>(rsp)) {
                    auto const& value = std::get<ocpp2_0::BootNotificationResponse>(rsp);
                    auto const& ts = value.currentTime.getTimestamp();

                    if (ts.has_value()) {
                        system_interface_->setSystemClock(ts.value());
                    } else {
                        CHARGELAB_LOG_MESSAGE(warning) << "Invalid BootNotification response timestamp: "
                                                       << value.currentTime;
                    }

                    CHARGELAB_LOG_MESSAGE(info) << "BootNotification response was: " << value;
                    switch (value.status) {
                        case ocpp2_0::RegistrationStatusEnumType::kAccepted:
                            requested_next_boot_notification_req_ = std::nullopt;
                            settings_->HeartbeatInterval.setValue(value.interval);
                            break;

                        case ocpp2_0::RegistrationStatusEnumType::kValueNotFoundInEnum:
                            CHARGELAB_LOG_MESSAGE(error) << "Invalid BootNotification response status for request ID: " << unique_id;
                            [[fallthrough]];

                        case ocpp2_0::RegistrationStatusEnumType::kPending:
                        case ocpp2_0::RegistrationStatusEnumType::kRejected:
                            requested_next_boot_notification_req_ = static_cast<SteadyPointMillis> (
                                    system_interface_->steadyClockNow() + value.interval*1000
                            );
                            break;
                    }
                    switch (value.status) {
                        case ocpp2_0::RegistrationStatusEnumType::kRejected:
                        case ocpp2_0::RegistrationStatusEnumType::kValueNotFoundInEnum:
                            registration_complete_ = false;
                            allow_ocpp_calls_ = false;
                            break;

                        case ocpp2_0::RegistrationStatusEnumType::kPending:
                            registration_complete_ = false;
                            allow_ocpp_calls_ = true;
                            break;

                        case ocpp2_0::RegistrationStatusEnumType::kAccepted:
                            registration_complete_ = true;
                            allow_ocpp_calls_ = true;
                            break;
                    }
                } else {
                    CHARGELAB_LOG_MESSAGE(warning) << "Error response to BootNotification request: " << std::get<ocpp2_0::CallError> (rsp);
                }
            }
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::TriggerMessageResponse>>
        onTriggerMessageReq(const ocpp2_0::TriggerMessageRequest& req) override {
            switch (req.requestedMessage) {
                case ocpp2_0::MessageTriggerEnumType::kBootNotification:
                    if (!registration_complete_) {
                        force_boot_notification_req_ = true;
                        return ocpp2_0::TriggerMessageResponse{ocpp2_0::TriggerMessageStatusEnumType::kAccepted};
                    } else {
                        // F06.FR.17
                        return ocpp2_0::TriggerMessageResponse{ocpp2_0::TriggerMessageStatusEnumType::kRejected};
                    }

                default:
                    return unauthorizedCallHandler2_0();
            }
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::CancelReservationResponse>>
        onCancelReservationReq(const ocpp2_0::CancelReservationRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::CertificateSignedResponse>>
        onCertificateSignedReq(const ocpp2_0::CertificateSignedRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::ChangeAvailabilityResponse>>
        onChangeAvailabilityReq(const ocpp2_0::ChangeAvailabilityRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::ClearCacheResponse>>
        onClearCacheReq(const ocpp2_0::ClearCacheRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::ClearChargingProfileResponse>>
        onClearChargingProfileReq(const ocpp2_0::ClearChargingProfileRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::ClearDisplayMessageResponse>>
        onClearDisplayMessageReq(const ocpp2_0::ClearDisplayMessageRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::ClearVariableMonitoringResponse>>
        onClearVariableMonitoringReq(const ocpp2_0::ClearVariableMonitoringRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::CostUpdatedResponse>>
        onCostUpdatedReq(const ocpp2_0::CostUpdatedRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::CustomerInformationResponse>>
        onCustomerInformationReq(const ocpp2_0::CustomerInformationRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::DataTransferResponse>>
        onDataTransferReq(const ocpp2_0::DataTransferRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::DeleteCertificateResponse>>
        onDeleteCertificateReq(const ocpp2_0::DeleteCertificateRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetBaseReportResponse>>
        onGetBaseReportReq(const ocpp2_0::GetBaseReportRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetChargingProfilesResponse>>
        onGetChargingProfilesReq(const ocpp2_0::GetChargingProfilesRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetCompositeScheduleResponse>>
        onGetCompositeScheduleReq(const ocpp2_0::GetCompositeScheduleRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetDisplayMessagesResponse>>
        onGetDisplayMessagesReq(const ocpp2_0::GetDisplayMessagesRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetInstalledCertificateIdsResponse>>
        onGetInstalledCertificateIdsReq(const ocpp2_0::GetInstalledCertificateIdsRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetLocalListVersionResponse>>
        onGetLocalListVersionReq(const ocpp2_0::GetLocalListVersionRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetLogResponse>>
        onGetLogReq(const ocpp2_0::GetLogRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetMonitoringReportResponse>>
        onGetMonitoringReportReq(const ocpp2_0::GetMonitoringReportRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetReportResponse>>
        onGetReportReq(const ocpp2_0::GetReportRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetTransactionStatusResponse>>
        onGetTransactionStatusReq(const ocpp2_0::GetTransactionStatusRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetVariablesResponse>>
        onGetVariablesReq(const ocpp2_0::GetVariablesRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::InstallCertificateResponse>>
        onInstallCertificateReq(const ocpp2_0::InstallCertificateRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::PublishFirmwareResponse>>
        onPublishFirmwareReq(const ocpp2_0::PublishFirmwareRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::RequestStartTransactionResponse>>
        onRequestStartTransactionReq(const ocpp2_0::RequestStartTransactionRequest&) override {
            // B02.FR.05
            if (!registration_complete_) {
                return ocpp2_0::RequestStartTransactionResponse {ocpp2_0::RequestStartStopStatusEnumType::kRejected};
            }

            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::RequestStopTransactionResponse>>
        onRequestStopTransactionReq(const ocpp2_0::RequestStopTransactionRequest&) override {
            // B02.FR.05
            if (!registration_complete_) {
                return ocpp2_0::RequestStopTransactionResponse {ocpp2_0::RequestStartStopStatusEnumType::kRejected};
            }

            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::ReserveNowResponse>>
        onReserveNowReq(const ocpp2_0::ReserveNowRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::ResetResponse>>
        onResetReq(const ocpp2_0::ResetRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::SendLocalListResponse>>
        onSendLocalListReq(const ocpp2_0::SendLocalListRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::SetChargingProfileResponse>>
        onSetChargingProfileReq(const ocpp2_0::SetChargingProfileRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::SetDisplayMessageResponse>>
        onSetDisplayMessageReq(const ocpp2_0::SetDisplayMessageRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::SetMonitoringBaseResponse>>
        onSetMonitoringBaseReq(const ocpp2_0::SetMonitoringBaseRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::SetMonitoringLevelResponse>>
        onSetMonitoringLevelReq(const ocpp2_0::SetMonitoringLevelRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::SetNetworkProfileResponse>>
        onSetNetworkProfileReq(const ocpp2_0::SetNetworkProfileRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::SetVariableMonitoringResponse>>
        onSetVariableMonitoringReq(const ocpp2_0::SetVariableMonitoringRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::SetVariablesResponse>>
        onSetVariablesReq(const ocpp2_0::SetVariablesRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::UnlockConnectorResponse>>
        onUnlockConnectorReq(const ocpp2_0::UnlockConnectorRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::UnpublishFirmwareResponse>>
        onUnpublishFirmwareReq(const ocpp2_0::UnpublishFirmwareRequest&) override {
            return unauthorizedCallHandler2_0();
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::UpdateFirmwareResponse>>
        onUpdateFirmwareReq(const ocpp2_0::UpdateFirmwareRequest&) override {
            return unauthorizedCallHandler2_0();
        }

    private:
        template <typename T>
        void runStepImpl(T&& sendBootNotification) {
            auto const now = system_interface_->steadyClockNow();
            if (connection_transition_timeout_.has_value()) {
                if (allow_ocpp_calls_) {
                    CHARGELAB_LOG_MESSAGE(debug) << "Connection transition completed - committing";
                    connection_transition_timeout_ = std::nullopt;
                    settings_->commit(SettingTransitionType::kConnection);
                } else if (now >= connection_transition_timeout_.value()) {
                    CHARGELAB_LOG_MESSAGE(debug) << "Connection transition failed - rolling back to previous settings";
                    settings_->rollback(SettingTransitionType::kConnection);
                    settings_->saveIfModified();
                    system_interface_->resetHard();
                }
            }

            // Note: only updating the security profile when first connecting to the back-end
            if (!security_profile_checked_ && allow_ocpp_calls_ && !settings_->hasRunningTransition(SettingTransitionType::kConnection)) {
                auto const activeNetworkProfileSlot = settings_->ActiveNetworkProfile.getValue();
                auto const activeNetworkProfile = settings_->NetworkConnectionProfiles.transitionCurrentValue(activeNetworkProfileSlot);
                if (activeNetworkProfile.has_value() &&
                    activeNetworkProfile->securityProfile > settings_->SecurityProfile.getValue()) {
                    // A05.FR.06
                    for (std::size_t i = 0; i < settings_->NetworkConnectionProfiles.transitionSize(); i++) {
                        auto const value = settings_->NetworkConnectionProfiles.transitionCurrentValue(i);
                        if (!value.has_value())
                            continue;
                        if (value->securityProfile >= activeNetworkProfile->securityProfile)
                            continue;

                        CHARGELAB_LOG_MESSAGE(info) << "Removing profile " << i << ": " << value;
                        settings_->NetworkConnectionProfiles.forceCurrentValue(i, std::nullopt);

                        char const* ifs = "";
                        std::string filtered_priorities;
                        string::SplitVisitor(settings_->NetworkConfigurationPriority.transitionCurrentValue(), ",", [&](std::string const& value) {
                            if (string::ToInteger(value) != std::make_optional(i)) {
                                filtered_priorities += ifs + value;
                                ifs = ",";
                            }
                        });

                        CHARGELAB_LOG_MESSAGE(info) << "Priorities before: " << settings_->NetworkConfigurationPriority.transitionCurrentValue();
                        settings_->NetworkConfigurationPriority.forceValue(filtered_priorities);
                        CHARGELAB_LOG_MESSAGE(info) << "Priorities after: " << settings_->NetworkConfigurationPriority.transitionCurrentValue();
                    }

                    settings_->SecurityProfile.setValue(activeNetworkProfile->securityProfile);
                    CHARGELAB_LOG_MESSAGE(info) << "Security profile updated to: " << activeNetworkProfile->securityProfile;
                }

                security_profile_checked_ = true;
            }

            if (force_boot_notification_req_) {
                force_boot_notification_req_ = false;
                pending_boot_notification_req_.setWithTimeout(
                        settings_->DefaultMessageTimeout.getValue(),
                        sendBootNotification()
                );
                return;
            }

            if (registrationComplete())
                return;

            if (requested_next_boot_notification_req_.has_value()) {
                if (now - requested_next_boot_notification_req_.value() < 0)
                    return;
            }

            if (pending_boot_notification_req_.wasIdleFor(settings_->HeartbeatInterval.getValue())) {
                pending_boot_notification_req_.setWithTimeout(
                        settings_->DefaultMessageTimeout.getValue(),
                        sendBootNotification()
                );
            }
        }

        std::optional<ocpp1_6::CallError> unauthorizedCallHandler1_6() {
            if (allow_ocpp_calls_) {
                return std::nullopt;
            } else {
                return unauthorizedError1_6();
            }
        }

        std::optional<ocpp2_0::CallError> unauthorizedCallHandler2_0() {
            if (allow_ocpp_calls_) {
                return std::nullopt;
            } else {
                return unauthorizedError2_0();
            }
        }

        static ocpp1_6::CallError unauthorizedError1_6() {
            return ocpp1_6::CallError {
                    ocpp1_6::ErrorCode::kSecurityError,
                    "Waiting for CSMS to accept boot notification",
                    common::RawJson::empty_object()
            };
        }

        static ocpp2_0::CallError unauthorizedError2_0() {
            return ocpp2_0::CallError {
                ocpp2_0::ErrorCode::kSecurityError,
                "Waiting for CSMS to accept boot notification",
                common::RawJson::empty_object()
            };
        }

    private:
        std::shared_ptr<Settings> settings_;
        std::shared_ptr<SystemInterface> system_interface_;
        OperationHolder<std::string> pending_boot_notification_req_;

        std::optional<SteadyPointMillis> connection_transition_timeout_ = std::nullopt;
        std::optional<SteadyPointMillis> requested_next_boot_notification_req_ = std::nullopt;
        std::atomic<bool> registration_complete_ = false;
        std::atomic<bool> allow_ocpp_calls_ = false;
        std::atomic<bool> force_boot_notification_req_ = false;
        std::atomic<bool> security_profile_checked_ = false;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_BOOT_NOTIFICATION_MODULE_H
