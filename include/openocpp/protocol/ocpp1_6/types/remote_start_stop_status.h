#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_REMOTE_START_STOP_STATUS_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_REMOTE_START_STOP_STATUS_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(RemoteStartStopStatus, 
        Accepted,
        Rejected
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_REMOTE_START_STOP_STATUS_H
