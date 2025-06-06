#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_COST_UPDATED_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_COST_UPDATED_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct CostUpdatedRequest {
        double totalCost {};
        IdentifierStringPrimitive<36> transactionId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(CostUpdatedRequest, kCostUpdated, totalCost, transactionId)
    };


    struct CostUpdatedResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(CostUpdatedResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_COST_UPDATED_H
