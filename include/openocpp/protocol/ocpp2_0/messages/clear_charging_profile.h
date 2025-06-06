#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_CHARGING_PROFILE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_CHARGING_PROFILE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/clear_charging_profile_type.h"
#include "openocpp/protocol/ocpp2_0/types/clear_charging_profile_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ClearChargingProfileRequest {
        std::optional<int> chargingProfileId {};
        std::optional<ClearChargingProfileType> chargingProfileCriteria {};
        CHARGELAB_JSON_INTRUSIVE_CALL(ClearChargingProfileRequest, kClearChargingProfile, chargingProfileId, chargingProfileCriteria)
    };


    struct ClearChargingProfileResponse {
        ClearChargingProfileStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(ClearChargingProfileResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_CHARGING_PROFILE_H
