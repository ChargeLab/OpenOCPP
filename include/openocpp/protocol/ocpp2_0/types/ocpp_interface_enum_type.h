#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_INTERFACE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_INTERFACE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(OCPPInterfaceEnumType, 
        Wired0,
        Wired1,
        Wired2,
        Wired3,
        Wireless0,
        Wireless1,
        Wireless2,
        Wireless3
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_INTERFACE_ENUM_TYPE_H
