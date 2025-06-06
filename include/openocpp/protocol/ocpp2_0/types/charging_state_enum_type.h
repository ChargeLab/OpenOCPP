#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_STATE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_STATE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(ChargingStateEnumType, 
        Charging,
        EVConnected,
        SuspendedEV,
        SuspendedEVSE,
        Idle
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_STATE_ENUM_TYPE_H
