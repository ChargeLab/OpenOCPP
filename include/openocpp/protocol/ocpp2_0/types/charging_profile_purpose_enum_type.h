#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_PURPOSE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_PURPOSE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(ChargingProfilePurposeEnumType, 
        ChargingStationExternalConstraints,
        ChargingStationMaxProfile,
        TxDefaultProfile,
        TxProfile
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_PURPOSE_ENUM_TYPE_H
