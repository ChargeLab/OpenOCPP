#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_UNIT_OF_MEASURE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_UNIT_OF_MEASURE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct UnitOfMeasureType {
        std::optional<StringPrimitive<20>> unit {};
        std::optional<int> multiplier {};
        CHARGELAB_JSON_INTRUSIVE(UnitOfMeasureType, unit, multiplier)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_UNIT_OF_MEASURE_TYPE_H
