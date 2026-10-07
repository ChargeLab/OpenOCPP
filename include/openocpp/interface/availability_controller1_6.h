#pragma once

#include "openocpp/protocol/ocpp1_6/messages/change_availability.h"

namespace chargelab {

    /**
     * Optional interface for observing OCPP 1.6 ChangeAvailability requests from the backend.
     *
     * ConnectorStatusModule applies a ChangeAvailability.req itself (marks the connectors inoperative or operative and
     * answers the backend), then notifies the controller. This is used when OpenOCPP represents another charger, which
     * should apply the change too.
     */
    class AvailabilityController1_6 {
    public:
        virtual ~AvailabilityController1_6() = default;

        /**
         * Called after a ChangeAvailability.req from the backend was applied. Does not affect the response.
         */
        virtual void onChangeAvailability(const ocpp1_6::ChangeAvailabilityReq& request) = 0;
    };

}  // namespace chargelab
