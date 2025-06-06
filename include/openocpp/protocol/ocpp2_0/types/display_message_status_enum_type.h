#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_DISPLAY_MESSAGE_STATUS_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_DISPLAY_MESSAGE_STATUS_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(DisplayMessageStatusEnumType, 
        Accepted,
        NotSupportedMessageFormat,
        Rejected,
        NotSupportedPriority,
        NotSupportedState,
        UnknownTransaction
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_DISPLAY_MESSAGE_STATUS_ENUM_TYPE_H
