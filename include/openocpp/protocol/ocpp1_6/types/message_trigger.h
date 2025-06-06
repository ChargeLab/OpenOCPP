#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_MESSAGE_TRIGGER_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_MESSAGE_TRIGGER_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(MessageTrigger, 
        BootNotification,
        DiagnosticsStatusNotification,
        FirmwareStatusNotification,
        Heartbeat,
        MeterValues,
        StatusNotification
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_MESSAGE_TRIGGER_H
