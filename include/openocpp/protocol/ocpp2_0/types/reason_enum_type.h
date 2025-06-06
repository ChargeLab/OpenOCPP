#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_REASON_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_REASON_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(ReasonEnumType, 
        DeAuthorized,
        EmergencyStop,
        EnergyLimitReached,
        EVDisconnected,
        GroundFault,
        ImmediateReset,
        Local,
        LocalOutOfCredit,
        MasterPass,
        Other,
        OvercurrentFault,
        PowerLoss,
        PowerQuality,
        Reboot,
        Remote,
        SOCLimitReached,
        StoppedByEV,
        TimeLimitReached,
        Timeout
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_REASON_ENUM_TYPE_H
