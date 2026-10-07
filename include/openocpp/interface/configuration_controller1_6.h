#pragma once

#include <functional>
#include <optional>

#include "openocpp/protocol/ocpp1_6/messages/change_configuration.h"
#include "openocpp/protocol/ocpp1_6/types/key_value.h"

namespace chargelab {

    /**
     * Optional interface for taking part in OCPP 1.6 ChangeConfiguration and GetConfiguration requests from the backend.
     *
     * This is used when OpenOCPP represents another charger: configuration keys that belong to that charger are
     * answered by the controller, the others by OpenOCPP's own settings.
     */
    class ConfigurationController1_6 {
    public:
        virtual ~ConfigurationController1_6() = default;

        /**
         * Called first for a ChangeConfiguration.req from the backend.
         *
         * @return the status to answer with, in which case OpenOCPP does not change its own settings; or nullopt to let
         *         OpenOCPP handle the request as usual.
         */
        virtual std::optional<ocpp1_6::ConfigurationStatus> onChangeConfiguration(
                const ocpp1_6::ChangeConfigurationReq& request) = 0;

        /**
         * Called after OpenOCPP changed one of its own settings for a ChangeConfiguration.req, with the status it
         * answered: Accepted, or RebootRequired if the change takes effect after a restart.
         */
        virtual void onConfigurationChanged(
                const ocpp1_6::ChangeConfigurationReq& request,
                ocpp1_6::ConfigurationStatus status) {
        }

        /**
         * Visits the configuration keys the controller answers for in GetConfiguration.req. They take precedence over
         * OpenOCPP settings with the same key.
         */
        virtual void visitConfiguration(std::function<void(ocpp1_6::KeyValue const&)> const& visitor) = 0;
    };

}  // namespace chargelab
