#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_TRIGGER_MESSAGE_STATUS_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_TRIGGER_MESSAGE_STATUS_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(TriggerMessageStatus,
        Accepted,
        Rejected,
        NotImplemented
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_TRIGGER_MESSAGE_STATUS_H
