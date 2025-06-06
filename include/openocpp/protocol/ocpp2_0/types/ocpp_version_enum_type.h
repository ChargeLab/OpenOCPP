#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_VERSION_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_VERSION_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(OCPPVersionEnumType, 
        OCPP12,
        OCPP15,
        OCPP16,
        OCPP20
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_OCPP_VERSION_ENUM_TYPE_H
