#pragma once

#include <memory>

#include "openocpp/interface/authorization_listener2_0.h"
#include "openocpp/interface/transaction_listener2_0.h"
#include "openocpp/interface/transaction_controller2_0.h"

namespace chargelab {

    /**
     * Optional interfaces for integrating with OCPP 2.0.1 charger business behavior.
     *
     * These interfaces allow external components to observe charger business events
     * and optionally influence specific business decisions.
     */
    struct ChargerInterfaces2_0 {
        std::weak_ptr<TransactionListener2_0> transaction_listener;
        std::weak_ptr<AuthorizationListener2_0> authorization_listener;
        std::weak_ptr<TransactionController2_0> transacdtion_controller;
    };

}  // namespace chargelab