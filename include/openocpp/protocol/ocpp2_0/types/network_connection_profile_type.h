#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_NETWORK_CONNECTION_PROFILE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_NETWORK_CONNECTION_PROFILE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/ocpp_version_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/ocpp_transport_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/ocpp_interface_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/vpn_type.h"
#include "openocpp/protocol/ocpp2_0/types/apn_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct NetworkConnectionProfileType {
        OCPPVersionEnumType ocppVersion {};
        OCPPTransportEnumType ocppTransport {};
        StringPrimitive<512> ocppCsmsUrl {};
        int messageTimeout {};
        int securityProfile {};
        OCPPInterfaceEnumType ocppInterface {};
        std::optional<VPNType> vpn {};
        std::optional<APNType> apn {};
        CHARGELAB_JSON_INTRUSIVE(NetworkConnectionProfileType, ocppVersion, ocppTransport, ocppCsmsUrl, messageTimeout, securityProfile, ocppInterface, vpn, apn)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_NETWORK_CONNECTION_PROFILE_TYPE_H
