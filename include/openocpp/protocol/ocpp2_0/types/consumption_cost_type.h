#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CONSUMPTION_COST_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CONSUMPTION_COST_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/cost_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ConsumptionCostType {
        double startValue {};
        std::vector<CostType> cost {};
        CHARGELAB_JSON_INTRUSIVE(ConsumptionCostType, startValue, cost)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CONSUMPTION_COST_TYPE_H
