#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_REASON_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_REASON_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(Reason, 
        DeAuthorized,
        EmergencyStop,
        EVDisconnected,
        HardReset,
        Local,
        Other,
        PowerLoss,
        Reboot,
        Remote,
        SoftReset,
        UnlockCommand,
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_REASON_H
