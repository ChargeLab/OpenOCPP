#pragma once

#include "openocpp/protocol/ocpp2_0/messages/reset.h"

namespace chargelab {

    /**
     * Optional interface for taking over OCPP 2.0.1 Reset requests from the backend.
     *
     * When a controller is set, ResetModule passes the Reset request to it instead of
     * scheduling a reset of this system. This is used when OpenOCPP represents
     * another charger, which is the one that has to be reset.
     */
    class ResetController2_0 {
    public:
        virtual ~ResetController2_0() = default;

        /**
         * Called for a Reset request from the backend.
         *
         * @return the status to answer with.
         */
        virtual ocpp2_0::ResetStatusEnumType onReset(const ocpp2_0::ResetRequest& request) = 0;
    };

}  // namespace chargelab
