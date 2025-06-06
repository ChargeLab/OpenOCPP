#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_CHARACTERISTICS_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_CHARACTERISTICS_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/data_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct VariableCharacteristicsType {
        std::optional<StringPrimitive<16>> unit {};
        DataEnumType dataType {};
        std::optional<double> minLimit {};
        std::optional<double> maxLimit {};
        std::optional<StringPrimitive<1000>> valuesList {};
        bool supportsMonitoring {};
        CHARGELAB_JSON_INTRUSIVE(VariableCharacteristicsType, unit, dataType, minLimit, maxLimit, valuesList, supportsMonitoring)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_CHARACTERISTICS_TYPE_H
