#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_VPN_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_VPN_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/vpn_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct VPNType {
        StringPrimitive<512> server {};
        StringPrimitive<20> user {};
        std::optional<StringPrimitive<20>> group {};
        StringPrimitive<20> password {};
        StringPrimitive<255> key {};
        VPNEnumType type {};
        CHARGELAB_JSON_INTRUSIVE(VPNType, server, user, group, password, key, type)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_VPN_TYPE_H
