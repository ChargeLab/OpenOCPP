#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_STATE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_STATE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(MessageStateEnumType, 
        Charging,
        Faulted,
        Idle,
        Unavailable
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_STATE_ENUM_TYPE_H
