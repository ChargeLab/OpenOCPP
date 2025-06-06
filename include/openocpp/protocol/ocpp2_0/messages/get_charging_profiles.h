#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_CHARGING_PROFILES_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_CHARGING_PROFILES_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/charging_profile_criterion_type.h"
#include "openocpp/protocol/ocpp2_0/types/get_charging_profile_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetChargingProfilesRequest {
        int requestId {};
        std::optional<int> evseId {};
        ChargingProfileCriterionType chargingProfile {};
        CHARGELAB_JSON_INTRUSIVE_CALL(GetChargingProfilesRequest, kGetChargingProfiles, requestId, evseId, chargingProfile)
    };


    struct GetChargingProfilesResponse {
        GetChargingProfileStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(GetChargingProfilesResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_CHARGING_PROFILES_H
