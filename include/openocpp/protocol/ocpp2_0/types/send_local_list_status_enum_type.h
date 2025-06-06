#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SEND_LOCAL_LIST_STATUS_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SEND_LOCAL_LIST_STATUS_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(SendLocalListStatusEnumType, 
        Accepted,
        Failed,
        VersionMismatch
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SEND_LOCAL_LIST_STATUS_ENUM_TYPE_H
