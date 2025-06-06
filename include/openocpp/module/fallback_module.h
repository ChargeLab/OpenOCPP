#ifndef CHARGELAB_OPEN_FIRMWARE_FALLBACK_MODULE_H
#define CHARGELAB_OPEN_FIRMWARE_FALLBACK_MODULE_H

#include "openocpp/module/common_templates.h"

namespace chargelab {
    class FallbackModule : public ServiceStatefulGeneral {
    public:
        FallbackModule(std::shared_ptr<SystemInterface> system)
            : system_(std::move(system))
        {
        }

    private:
        void runStep(ocpp1_6::OcppRemote&) override {
        }

        void runStep(ocpp2_0::OcppRemote &remote) override {
            if (pending_customer_report_.has_value()) {
                auto id = remote.sendNotifyCustomerInformationReq(ocpp2_0::NotifyCustomerInformationRequest {
                    "",
                    false,
                    0,
                    system_->systemClockNow(),
                    pending_customer_report_->requestId
                });

                if (id.has_value()) {
                    pending_customer_report_ = std::nullopt;
                }
            }
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::TriggerMessageRsp>>
        onTriggerMessageReq(const ocpp1_6::TriggerMessageReq&) override {
            return ocpp1_6::TriggerMessageRsp {ocpp1_6::TriggerMessageStatus::kNotImplemented};
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::TriggerMessageResponse>>
        onTriggerMessageReq(const ocpp2_0::TriggerMessageRequest&) override {
            // F06.FR.08
            return ocpp2_0::TriggerMessageResponse {ocpp2_0::TriggerMessageStatusEnumType::kNotImplemented};
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::DataTransferRsp>>
        onDataTransferReq(const ocpp1_6::DataTransferReq&) override {
            return ocpp1_6::DataTransferRsp {ocpp1_6::DataTransferStatus::kUnknownVendorId};
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::DataTransferResponse>>
        onDataTransferReq(const ocpp2_0::DataTransferRequest&) override {
            // P01.FR.05
            return ocpp2_0::DataTransferResponse {ocpp2_0::DataTransferStatusEnumType::kUnknownVendorId};
        }

        // TODO: This operation doesn't seem relevant if we're not caching customer identifiers like authorization
        //  tokens in any sort of local list or authorization cache. Leaving this here for now.
        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::CustomerInformationResponse>>
        onCustomerInformationReq(const ocpp2_0::CustomerInformationRequest& request) override {
            // N10.FR.05
            if (pending_customer_report_.has_value())
                return ocpp2_0::CustomerInformationResponse {ocpp2_0::CustomerInformationStatusEnumType::kRejected};

            // N10.FR.07
            if (!request.clear && !request.report)
                return ocpp2_0::CustomerInformationResponse {ocpp2_0::CustomerInformationStatusEnumType::kRejected};

            // N10.FR.01
            pending_customer_report_ = request;
            return ocpp2_0::CustomerInformationResponse {ocpp2_0::CustomerInformationStatusEnumType::kAccepted};
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::UnlockConnectorRsp>>
        onUnlockConnectorReq(const ocpp1_6::UnlockConnectorReq&) override {
            return ocpp1_6::UnlockConnectorRsp {ocpp1_6::UnlockStatus::kNotSupported};
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::UnlockConnectorResponse>>
        onUnlockConnectorReq(const ocpp2_0::UnlockConnectorRequest&) override {
            // Note - responding here to satisfy TC_E_16_CS, however a NotImplemented default response seems more
            // appropriate.
            return ocpp2_0::UnlockConnectorResponse {ocpp2_0::UnlockStatusEnumType::kUnlockFailed};
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ClearCacheRsp>>
        onClearCacheReq(const ocpp1_6::ClearCacheReq&) override {
            return ocpp1_6::ClearCacheRsp {ocpp1_6::ClearCacheStatus::kAccepted};
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::ClearCacheResponse>>
        onClearCacheReq(const ocpp2_0::ClearCacheRequest&) override {
            return ocpp2_0::ClearCacheResponse {ocpp2_0::ClearCacheStatusEnumType::kAccepted};
        }

    private:
        std::shared_ptr<SystemInterface> system_;

        std::optional<ocpp2_0::CustomerInformationRequest> pending_customer_report_ = std::nullopt;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_FALLBACK_MODULE_H
