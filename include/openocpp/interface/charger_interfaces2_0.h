#pragma once

#include <memory>

#include "openocpp/interface/authorization_listener2_0.h"
#include "openocpp/interface/availability_controller2_0.h"
#include "openocpp/interface/reset_controller2_0.h"
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
        std::weak_ptr<TransactionController2_0> transaction_controller;
        std::weak_ptr<ResetController2_0> reset_controller;
        std::weak_ptr<AvailabilityController2_0> availability_controller;
    };

}  // namespace chargelab