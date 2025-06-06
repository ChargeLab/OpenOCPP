#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_SET_CHARGING_PROFILE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_SET_CHARGING_PROFILE_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/charging_profile.h"
#include "openocpp/protocol/ocpp1_6/types/charging_profile_status.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct SetChargingProfileReq {
        int connectorId;
        ChargingProfile csChargingProfiles;
        CHARGELAB_JSON_INTRUSIVE_CALL(SetChargingProfileReq, kSetChargingProfile, connectorId, csChargingProfiles)
    };

    struct SetChargingProfileRsp {
        ChargingProfileStatus status;
        CHARGELAB_JSON_INTRUSIVE(SetChargingProfileRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_SET_CHARGING_PROFILE_H
