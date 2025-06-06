#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_TRIGGER_MESSAGE_STATUS_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_TRIGGER_MESSAGE_STATUS_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(TriggerMessageStatusEnumType, 
        Accepted,
        Rejected,
        NotImplemented
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_TRIGGER_MESSAGE_STATUS_ENUM_TYPE_H
