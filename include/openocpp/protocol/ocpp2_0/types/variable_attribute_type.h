#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_ATTRIBUTE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_ATTRIBUTE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/attribute_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/mutability_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct VariableAttributeType {
        std::optional<AttributeEnumType> type {};
        std::optional<StringPrimitive<2500>> value {};
        std::optional<MutabilityEnumType> mutability {};
        std::optional<bool> persistent {};
        std::optional<bool> constant {};
        CHARGELAB_JSON_INTRUSIVE(VariableAttributeType, type, value, mutability, persistent, constant)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_ATTRIBUTE_TYPE_H
