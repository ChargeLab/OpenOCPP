#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_MODEM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_MODEM_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ModemType {
        std::optional<IdentifierStringPrimitive<20>> iccid {};
        std::optional<IdentifierStringPrimitive<20>> imsi {};
        CHARGELAB_JSON_INTRUSIVE(ModemType, iccid, imsi)
    };

}


#endif //CHARGELAB_OPEN_FIRMWARE_2_0_MODEM_TYPE_H
