#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_CHARGING_PROFILE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_CHARGING_PROFILE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/charging_profile_purpose_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ClearChargingProfileType {
        std::optional<int> evseId {};
        std::optional<ChargingProfilePurposeEnumType> chargingProfilePurpose {};
        std::optional<int> stackLevel {};
        CHARGELAB_JSON_INTRUSIVE(ClearChargingProfileType, evseId, chargingProfilePurpose, stackLevel)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_CHARGING_PROFILE_TYPE_H
