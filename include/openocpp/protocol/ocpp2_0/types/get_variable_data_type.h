#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_VARIABLE_DATA_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_VARIABLE_DATA_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/attribute_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/component_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetVariableDataType {
        std::optional<AttributeEnumType> attributeType {};
        ComponentType component {};
        VariableType variable {};
        CHARGELAB_JSON_INTRUSIVE(GetVariableDataType, attributeType, component, variable)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_VARIABLE_DATA_TYPE_H
