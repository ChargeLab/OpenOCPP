#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CHANGE_AVAILABILITY_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CHANGE_AVAILABILITY_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/availability_type.h"
#include "openocpp/protocol/ocpp1_6/types/availability_status.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct ChangeAvailabilityReq {
        int connectorId;
        AvailabilityType type;
        CHARGELAB_JSON_INTRUSIVE_CALL(ChangeAvailabilityReq, kChangeAvailability, connectorId, type)
    };

    struct ChangeAvailabilityRsp {
        AvailabilityStatus status;
        CHARGELAB_JSON_INTRUSIVE(ChangeAvailabilityRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CHANGE_AVAILABILITY_H
