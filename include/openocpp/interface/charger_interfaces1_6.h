#pragma once

#include <memory>

#include "openocpp/interface/authorization_listener1_6.h"
#include "openocpp/interface/transaction_listener1_6.h"
#include "openocpp/interface/transaction_controller1_6.h"
#include "openocpp/interface/reset_controller1_6.h"

namespace chargelab {

    /**
     * Optional interfaces for integrating with OCPP 1.6 charger business behavior.
     *
     * These interfaces allow external components to observe charger business events
     * and optionally influence specific business decisions.
     */
    struct ChargerInterfaces1_6 {
        std::weak_ptr<TransactionListener1_6> transaction_listener;
        std::weak_ptr<AuthorizationListener1_6> authorization_listener;
        std::weak_ptr<TransactionController1_6> transaction_controller;
        std::weak_ptr<ResetController1_6> reset_controller;
    };

}  // namespace chargelab