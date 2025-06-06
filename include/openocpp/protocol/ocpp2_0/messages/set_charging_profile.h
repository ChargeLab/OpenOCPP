#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SET_CHARGING_PROFILE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SET_CHARGING_PROFILE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/charging_profile_type.h"
#include "openocpp/protocol/ocpp2_0/types/charging_profile_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SetChargingProfileRequest {
        int evseId {};
        ChargingProfileType chargingProfile {};
        CHARGELAB_JSON_INTRUSIVE_CALL(SetChargingProfileRequest, kSetChargingProfile, evseId, chargingProfile)
    };


    struct SetChargingProfileResponse {
        ChargingProfileStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(SetChargingProfileResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SET_CHARGING_PROFILE_H
