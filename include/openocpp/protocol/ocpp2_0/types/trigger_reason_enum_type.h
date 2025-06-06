#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_TRIGGER_REASON_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_TRIGGER_REASON_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(TriggerReasonEnumType, 
        Authorized,
        CablePluggedIn,
        ChargingRateChanged,
        ChargingStateChanged,
        Deauthorized,
        EnergyLimitReached,
        EVCommunicationLost,
        EVConnectTimeout,
        MeterValueClock,
        MeterValuePeriodic,
        TimeLimitReached,
        Trigger,
        UnlockCommand,
        StopAuthorized,
        EVDeparted,
        EVDetected,
        RemoteStop,
        RemoteStart,
        AbnormalCondition,
        SignedDataReceived,
        ResetCommand
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_TRIGGER_REASON_ENUM_TYPE_H
