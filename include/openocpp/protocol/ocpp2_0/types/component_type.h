#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_COMPONENT_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_COMPONENT_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/evse_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ComponentType {
        IdentifierStringPrimitive<50> name {};
        std::optional<IdentifierStringPrimitive<50>> instance {};
        std::optional<EVSEType> evse {};
        CHARGELAB_JSON_INTRUSIVE(ComponentType, name, instance, evse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_COMPONENT_TYPE_H
