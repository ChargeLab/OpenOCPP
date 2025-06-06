#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_BOOT_REASON_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_BOOT_REASON_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(BootReasonEnumType, 
        ApplicationReset,
        FirmwareUpdate,
        LocalReset,
        PowerUp,
        RemoteReset,
        ScheduledReset,
        Triggered,
        Unknown,
        Watchdog
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_BOOT_REASON_ENUM_TYPE_H
