#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CHARGE_POINT_STATUS_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CHARGE_POINT_STATUS_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(ChargePointStatus,
        Available,
        Preparing,
        Charging,
        SuspendedEVSE,
        SuspendedEV,
        Finishing,
        Reserved,
        Unavailable,
        Faulted,
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CHARGE_POINT_STATUS_H
