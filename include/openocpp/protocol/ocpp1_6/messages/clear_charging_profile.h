#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CLEAR_CHARGING_PROFILE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CLEAR_CHARGING_PROFILE_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/charging_profile_purpose_type.h"
#include "openocpp/protocol/ocpp1_6/types/clear_charging_profile_status.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct ClearChargingProfileReq {
        std::optional<int> id;
        std::optional<int> connectorId;
        std::optional<ChargingProfilePurposeType> chargingProfilePurpose;
        std::optional<int> stackLevel;
        CHARGELAB_JSON_INTRUSIVE_CALL(ClearChargingProfileReq, kClearChargingProfile, id, connectorId, chargingProfilePurpose, stackLevel)
    };

    struct ClearChargingProfileRsp {
        ClearChargingProfileStatus status;
        CHARGELAB_JSON_INTRUSIVE(ClearChargingProfileRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CLEAR_CHARGING_PROFILE_H
