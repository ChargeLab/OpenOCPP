#pragma once

#include <string>
#include <optional>
#include "openocpp/protocol/ocpp1_6/types/reason.h"

namespace chargelab {

    class TransactionController1_6 {
    public:
        virtual ~TransactionController1_6() = default;

        virtual bool isTransactionStartAllowed(
            int connector_id,
            const std::string& id_tag) = 0;

        virtual std::optional<chargelab::ocpp1_6::Reason> getTransactionStopReason(
            int connector_id) = 0;
    };
}