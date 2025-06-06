#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHANGE_AVAILABILITY_STATUS_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHANGE_AVAILABILITY_STATUS_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(ChangeAvailabilityStatusEnumType, 
        Accepted,
        Rejected,
        Scheduled
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHANGE_AVAILABILITY_STATUS_ENUM_TYPE_H
