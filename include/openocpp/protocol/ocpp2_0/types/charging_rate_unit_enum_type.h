#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_RATE_UNIT_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_RATE_UNIT_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(ChargingRateUnitEnumType, 
        W,
        A
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_RATE_UNIT_ENUM_TYPE_H
