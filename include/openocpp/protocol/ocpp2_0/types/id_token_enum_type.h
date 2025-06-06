#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_ID_TOKEN_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_ID_TOKEN_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(IdTokenEnumType, 
        Central,
        eMAID,
        ISO14443,
        ISO15693,
        KeyCode,
        Local,
        MacAddress,
        NoAuthorization
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_ID_TOKEN_ENUM_TYPE_H
