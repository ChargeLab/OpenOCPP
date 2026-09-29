#pragma once

#include <string>

#include "openocpp/protocol/ocpp2_0/messages/authorize.h"

namespace chargelab {

class AuthorizationListener2_0 {
public:
    virtual ~AuthorizationListener2_0() = default;

    virtual void onAuthorizationResult(const ocpp2_0::AuthorizeResponse& response) = 0;

    virtual void onAuthorizationError(const ocpp2_0::CallError & error) = 0;
};

}  // namespace chargelab