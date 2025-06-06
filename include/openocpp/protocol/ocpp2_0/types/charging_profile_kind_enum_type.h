#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_KIND_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_KIND_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(ChargingProfileKindEnumType, 
        Absolute,
        Recurring,
        Relative
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_KIND_ENUM_TYPE_H
