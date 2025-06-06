#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_LIMIT_SOURCE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_LIMIT_SOURCE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(ChargingLimitSourceEnumType, 
        EMS,
        Other,
        SO,
        CSO
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_LIMIT_SOURCE_ENUM_TYPE_H
