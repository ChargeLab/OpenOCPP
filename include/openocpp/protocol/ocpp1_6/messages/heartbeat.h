#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_HEARTBEAT_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_HEARTBEAT_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct HeartbeatReq {
        CHARGELAB_JSON_INTRUSIVE_EMPTY_CALL(HeartbeatReq, kHeartbeat)
    };

    struct HeartbeatRsp {
        DateTime currentTime;
        CHARGELAB_JSON_INTRUSIVE(HeartbeatRsp, currentTime)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_HEARTBEAT_H
