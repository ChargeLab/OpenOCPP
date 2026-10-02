#pragma once

#include <string>

#include "openocpp/protocol/ocpp1_6/messages/authorize.h"

namespace chargelab {

    /**
     * Listener for OCPP 1.6 authorization results.
     *
     * Implementations can use this interface to observe the result of an
     * authorization request after it has been processed by OpenOCPP.
     *
     * The listener is observational only and does not affect OpenOCPP's
     * authorization or transaction processing.
     */
    class AuthorizationListener1_6 {
    public:
        virtual ~AuthorizationListener1_6() = default;

        /**
         * Called when an authorization result is available.
         *
         * @param id_tag The ID tag from the original authorization request.
         * @param response The OCPP 1.6 Authorize response.
         */
        virtual void onAuthorizationResult(const std::string& id_tag, const ocpp1_6::AuthorizeRsp& response) = 0;

        /**
         * Called when an authorization error occurs.
         *
         * @param id_tag The ID tag from the original authorization request.
         * @param error The OCPP 1.6 CALLERROR.
         */
        virtual void onAuthorizationError(const std::string& id_tag, const ocpp1_6::CallError& error) = 0;
    };

}  // namespace chargelab