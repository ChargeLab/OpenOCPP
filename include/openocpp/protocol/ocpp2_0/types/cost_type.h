#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_COST_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_COST_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/cost_kind_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct CostType {
        CostKindEnumType costKind {};
        int amount {};
        std::optional<int> amountMultiplier {};
        CHARGELAB_JSON_INTRUSIVE(CostType, costKind, amount, amountMultiplier)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_COST_TYPE_H
