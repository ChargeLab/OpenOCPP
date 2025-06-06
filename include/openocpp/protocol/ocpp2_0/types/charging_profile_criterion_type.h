#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_CRITERION_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_CRITERION_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/charging_profile_purpose_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/charging_limit_source_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ChargingProfileCriterionType {
        std::optional<ChargingProfilePurposeEnumType> chargingProfilePurpose {};
        std::optional<int> stackLevel {};
        std::optional<std::vector<int>> chargingProfileId {};
        std::optional<std::vector<ChargingLimitSourceEnumType>> chargingLimitSource {};
        CHARGELAB_JSON_INTRUSIVE(ChargingProfileCriterionType, chargingProfilePurpose, stackLevel, chargingProfileId, chargingLimitSource)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_PROFILE_CRITERION_TYPE_H
