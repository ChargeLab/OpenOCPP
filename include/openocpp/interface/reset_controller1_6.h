#pragma once

#include "openocpp/protocol/ocpp1_6/messages/reset.h"

namespace chargelab {

    /**
     * Optional interface for taking over OCPP 1.6 Reset requests from the backend.
     *
     * When a controller is set, ResetModule passes Reset.req to it instead of
     * scheduling a reset of this system. This is used when OpenOCPP represents
     * another charger, which is the one that has to be reset.
     */
    class ResetController1_6 {
    public:
        virtual ~ResetController1_6() = default;

        /**
         * Called for a Reset.req from the backend.
         *
         * @return true to answer Accepted, false to answer Rejected.
         */
        virtual bool onReset(const ocpp1_6::ResetReq& request) = 0;
    };

}  // namespace chargelab
