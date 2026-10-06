#pragma once

#include <optional>
#include "openocpp/protocol/ocpp2_0/types/evse_type.h"

namespace chargelab {

    class TransactionController2_0 {
    public:
        virtual ~TransactionController2_0() = default;

        virtual bool isTransactionStartAllowed(const std::optional<ocpp2_0::EVSEType>& evse) = 0;


    };
}