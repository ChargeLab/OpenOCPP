#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct VariableType {
        IdentifierStringPrimitive<50> name {};
        std::optional<IdentifierStringPrimitive<50>> instance {};
        CHARGELAB_JSON_INTRUSIVE(VariableType, name, instance)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_TYPE_H
