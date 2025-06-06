#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_VPN_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_VPN_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(VPNEnumType, 
        IKEv2,
        IPSec,
        L2TP,
        PPTP
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_VPN_ENUM_TYPE_H
