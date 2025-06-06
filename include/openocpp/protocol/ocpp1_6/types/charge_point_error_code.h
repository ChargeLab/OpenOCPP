#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CHARGE_POINT_ERROR_CODE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CHARGE_POINT_ERROR_CODE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(ChargePointErrorCode,
        ConnectorLockFailure,
        EVCommunicationError,
        GroundFailure,
        HighTemperature,
        InternalError,
        LocalListConflict,
        NoError,
        OtherError,
        OverCurrentFailure,
        OverVoltage,
        PowerMeterFailure,
        PowerSwitchFailure,
        ReaderFailure,
        ResetFailure,
        UnderVoltage,
        WeakSignal
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CHARGE_POINT_ERROR_CODE_H
