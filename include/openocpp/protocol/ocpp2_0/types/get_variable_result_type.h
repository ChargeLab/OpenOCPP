#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_VARIABLE_RESULT_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_VARIABLE_RESULT_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/get_variable_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/attribute_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/component_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetVariableResultType {
        GetVariableStatusEnumType attributeStatus {};
        std::optional<AttributeEnumType> attributeType {};
        std::optional<StringPrimitive<2500>> attributeValue {};
        ComponentType component {};
        VariableType variable {};
        std::optional<StatusInfoType> attributeStatusInfo {};
        CHARGELAB_JSON_INTRUSIVE(GetVariableResultType, attributeStatus, attributeType, attributeValue, component, variable, attributeStatusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_VARIABLE_RESULT_TYPE_H
