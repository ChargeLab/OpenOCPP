#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_PROFILE_STATUS_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_PROFILE_STATUS_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(ChargingProfileStatus,
        Accepted,
        Rejected,
        NotSupported
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_PROFILE_STATUS_H
