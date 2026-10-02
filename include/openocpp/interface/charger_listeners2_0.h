#pragma once

#include <memory>

#include "openocpp/interface/authorization_listener2_0.h"
#include "openocpp/interface/transaction_listener2_0.h"

namespace chargelab {

    /**
     * Optional listeners for observing OCPP 2.0.1 charger business events.
     *
     * These listeners provide notifications about events processed by OpenOCPP
     * without changing OpenOCPP's normal business logic.
     */
    struct ChargerListeners2_0 {
        std::weak_ptr<TransactionListener2_0> transaction_listener;
        std::weak_ptr<AuthorizationListener2_0> authorization_listener;
    };

}  // namespace chargelab