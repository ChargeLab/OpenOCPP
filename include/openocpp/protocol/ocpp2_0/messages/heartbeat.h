#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_HEARTBEAT_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_HEARTBEAT_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct HeartbeatRequest {
        CHARGELAB_JSON_INTRUSIVE_EMPTY_CALL(HeartbeatRequest, kHeartbeat)
    };


    struct HeartbeatResponse {
        DateTime currentTime {};
        CHARGELAB_JSON_INTRUSIVE(HeartbeatResponse, currentTime)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_HEARTBEAT_H
