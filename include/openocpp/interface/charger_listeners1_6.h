#pragma once

#include <memory>

#include "openocpp/interface/authorization_listener1_6.h"
#include "openocpp/interface/transaction_listener1_6.h"

namespace chargelab {

    /**
     * Optional listeners for observing OCPP 1.6 charger business events.
     *
     * These listeners provide notifications about events processed by OpenOCPP
     * without changing OpenOCPP's normal business logic.
     */
    struct ChargerListeners1_6 {
        std::shared_ptr<TransactionListener1_6> transaction_listener;
        std::shared_ptr<AuthorizationListener1_6> authorization_listener;
    };

}  // namespace chargelab