#ifndef CHARGELAB_OPEN_FIRMWARE_RESET_MODULE_H
#define CHARGELAB_OPEN_FIRMWARE_RESET_MODULE_H

#include "openocpp/module/common_templates.h"
#include "openocpp/module/connector_status_module.h"
#include "openocpp/interface/component/system_interface.h"
#include "openocpp/common/settings.h"

namespace chargelab {
    class ResetModule : public ServiceStatefulGeneral {
    private:
        static constexpr int kMinimumResetDelayMillis = 1*1000; // 3 seconds

    public:
        ResetModule(
                std::shared_ptr<Settings> settings,
                std::shared_ptr<SystemInterface> system,
                std::shared_ptr<ConnectorStatusModule> connector_status_module
        )
                : settings_(std::move(settings)),
                  system_(std::move(system)),
                  connector_status_module_(std::move(connector_status_module))
        {
        }

        ~ResetModule() override {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting ResetModule";
        }

    public:
        [[nodiscard]] bool pendingReset() const {
            return hard_reset_threshold_.has_value();
        }

        /**
         * Resets the charger when it's no longer in use with the provided reason code. This method is thread safe.
         *
         * @param reason
         */
        void resetOnIdle(ocpp2_0::BootReasonEnumType reason) {
            if (hard_reset_requested_) {
                CHARGELAB_LOG_MESSAGE(info) << "Ignoring soft reset request - hard reset already scheduled";
                return;
            }

            reset_reason_ = reason;
            soft_reset_requested_ = true;
        }

        /**
         * Resets the charger immediately with the provided reason code. This method is thread safe.
         *
         * @param reason
         */
        void resetImmediately(ocpp2_0::BootReasonEnumType reason) {
            reset_reason_ = reason;
            hard_reset_requested_ = true;
        }

    private:
        void runStep(ocpp1_6::OcppRemote&) override {
            runStepCommon();
        }

        void runStep(ocpp2_0::OcppRemote&) override {
            runStepCommon();
        }

        std::optional<ocpp2_0::ResponseToRequest <ocpp2_0::ResetResponse>>
        onResetReq(const ocpp2_0::ResetRequest &req) override {
            // B11.FR.09
            if (req.evseId.has_value()) {
                return ocpp2_0::ResetResponse {ocpp2_0::ResetStatusEnumType::kRejected};
            }

            switch (req.type) {
                default:
                    CHARGELAB_LOG_MESSAGE(warning) << "Invalid reset request type - treating as OnIdle";
                    [[fallthrough]];

                case ocpp2_0::ResetEnumType::kOnIdle:
                    if (hard_reset_requested_) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Rejecting soft reset request - hard reset already scheduled";
                        return ocpp2_0::ResetResponse {ocpp2_0::ResetStatusEnumType::kRejected};
                    }
                    if (soft_reset_requested_) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Rejecting soft reset request - soft reset already scheduled";
                        return ocpp2_0::ResetResponse {ocpp2_0::ResetStatusEnumType::kRejected};
                    }

                    soft_reset_requested_ = true;

                    // B12.FR.01
                    if (connector_status_module_->isChargingEnabled()) {
                        reset_reason_ = ocpp2_0::BootReasonEnumType::kScheduledReset;
                        return ocpp2_0::ResetResponse {ocpp2_0::ResetStatusEnumType::kScheduled};
                    }
                    break;

                case ocpp2_0::ResetEnumType::kImmediate:
                    if (hard_reset_requested_) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Rejecting soft reset request - hard reset already scheduled";
                        return ocpp2_0::ResetResponse {ocpp2_0::ResetStatusEnumType::kRejected};
                    }

                    hard_reset_requested_ = true;
                    break;
            }

            reset_reason_ = ocpp2_0::BootReasonEnumType::kRemoteReset;
            return ocpp2_0::ResetResponse {ocpp2_0::ResetStatusEnumType::kAccepted};
        }

        std::optional<ocpp1_6::ResponseToRequest <ocpp1_6::ResetRsp>>
        onResetReq(const ocpp1_6::ResetReq &req) override {
            switch (req.type) {
                case ocpp1_6::ResetType::kValueNotFoundInEnum:
                    CHARGELAB_LOG_MESSAGE(warning) << "Invalid reset request type - treating as Soft";
                    [[fallthrough]];

                case ocpp1_6::ResetType::kSoft:
                    if (hard_reset_requested_) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Rejecting soft reset request - hard reset already scheduled";
                        return ocpp1_6::ResetRsp{ocpp1_6::ResetStatus::kRejected};
                    }
                    if (soft_reset_requested_) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Rejecting soft reset request - soft reset already scheduled";
                        return ocpp1_6::ResetRsp{ocpp1_6::ResetStatus::kRejected};
                    }

                    soft_reset_requested_ = true;
                    break;

                case ocpp1_6::ResetType::kHard:
                    if (hard_reset_requested_) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Rejecting soft reset request - hard reset already scheduled";
                        return ocpp1_6::ResetRsp{ocpp1_6::ResetStatus::kRejected};
                    }

                    hard_reset_requested_ = true;
                    break;
            }

            return ocpp1_6::ResetRsp {ocpp1_6::ResetStatus::kAccepted};
        }

    private:
        void runStepCommon() {
            ocpp2_0::BootReasonEnumType const reason = reset_reason_;

            auto const now = system_->steadyClockNow();
            bool running_transactions = connector_status_module_->isChargingEnabled();
            if (hard_reset_requested_) {
                if (!hard_reset_threshold_.has_value()) {
                    hard_reset_threshold_ = static_cast<SteadyPointMillis> (now + kMinimumResetDelayMillis);
                    connector_status_module_->setPendingReset(true, true);
                    CHARGELAB_LOG_MESSAGE(info) << "Hard reset requested - restarting in: " << kMinimumResetDelayMillis << " millis";
                }

                auto const delta = now - hard_reset_threshold_.value();
                if (delta >= 0) {
                    CHARGELAB_LOG_MESSAGE(info) << "Resetting system";
                    hard_reset_threshold_ = std::nullopt;
                    settings_->CustomBootReason.setValue(reason.to_string());
                    settings_->saveIfModified();
                    system_->resetHard();
                }
            } else if (soft_reset_requested_) {
                if (!running_transactions) {
                    if (!soft_reset_threshold_.has_value()) {
                        soft_reset_threshold_ = static_cast<SteadyPointMillis> (now + kMinimumResetDelayMillis);
                        connector_status_module_->setPendingReset(true, false);
                        CHARGELAB_LOG_MESSAGE(info) << "Soft reset requested - restarting in: " << kMinimumResetDelayMillis << " millis";
                    }

                    auto const delta = now - soft_reset_threshold_.value();
                    if (delta >= 0) {
                        CHARGELAB_LOG_MESSAGE(info) << "Resetting system";
                        soft_reset_threshold_ = std::nullopt;
                        settings_->CustomBootReason.setValue(reason.to_string());
                        settings_->saveIfModified();
                        system_->resetSoft();
                    }
                } else {
                    soft_reset_threshold_ = std::nullopt;
                    // Tell the transaction module to stop transactions indirectly
                    connector_status_module_->setPendingReset(true, false);

                    CHARGELAB_LOG_MESSAGE(trace) << "Reset pending - waiting for running transactions to complete";
                }
            }
        }

    private:
        std::shared_ptr<Settings> settings_;
        std::shared_ptr<SystemInterface> system_;
        std::shared_ptr<ConnectorStatusModule> connector_status_module_;

        std::optional<SteadyPointMillis> hard_reset_threshold_ = std::nullopt;
        std::optional<SteadyPointMillis> soft_reset_threshold_ = std::nullopt;
        std::atomic<ocpp2_0::BootReasonEnumType> reset_reason_ = {ocpp2_0::BootReasonEnumType::kUnknown};
        std::atomic<bool> soft_reset_requested_ = false;
        std::atomic<bool> hard_reset_requested_ = false;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_RESET_MODULE_H
