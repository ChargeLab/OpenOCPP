#pragma once

#include "openocpp/protocol/ocpp2_0/messages/change_availability.h"

namespace chargelab {

    /**
     * Optional interface for observing OCPP 2.0.1 ChangeAvailability requests from the backend.
     *
     * ConnectorStatusModule applies a ChangeAvailabilityRequest itself (marks the EVSEs or connectors inoperative or
     * operative and answers the backend), then notifies the controller. This is used when OpenOCPP represents another
     * charger, which should apply the change too.
     */
    class AvailabilityController2_0 {
    public:
        virtual ~AvailabilityController2_0() = default;

        /**
         * Called after a ChangeAvailabilityRequest from the backend was applied. Does not affect the response.
         */
        virtual void onChangeAvailability(const ocpp2_0::ChangeAvailabilityRequest& request) = 0;
    };

}  // namespace chargelab
