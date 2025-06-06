#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SET_VARIABLE_RESULT_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SET_VARIABLE_RESULT_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/attribute_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/set_variable_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/component_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SetVariableResultType {
        std::optional<AttributeEnumType> attributeType {};
        SetVariableStatusEnumType attributeStatus {};
        ComponentType component {};
        VariableType variable {};
        std::optional<StatusInfoType> attributeStatusInfo {};
        CHARGELAB_JSON_INTRUSIVE(SetVariableResultType, attributeType, attributeStatus, component, variable, attributeStatusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SET_VARIABLE_RESULT_TYPE_H
