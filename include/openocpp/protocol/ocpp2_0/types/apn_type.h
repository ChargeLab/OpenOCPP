#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_APN_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_APN_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/apn_authentication_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct APNType {
        StringPrimitive<512> apn {};
        std::optional<StringPrimitive<20>> apnUserName {};
        std::optional<StringPrimitive<20>> apnPassword {};
        std::optional<int> simPin {};
        std::optional<IdentifierStringPrimitive<6>> preferredNetwork {};
        std::optional<bool> useOnlyPreferredNetwork {};
        APNAuthenticationEnumType apnAuthentication {};
        CHARGELAB_JSON_INTRUSIVE(APNType, apn, apnUserName, apnPassword, simPin, preferredNetwork, useOnlyPreferredNetwork, apnAuthentication)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_APN_TYPE_H
