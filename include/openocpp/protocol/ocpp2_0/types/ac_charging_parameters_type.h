#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_AC_CHARGING_PARAMETERS_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_AC_CHARGING_PARAMETERS_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ACChargingParametersType {
        int energyAmount {};
        int evMinCurrent {};
        int evMaxCurrent {};
        int evMaxVoltage {};
        CHARGELAB_JSON_INTRUSIVE(ACChargingParametersType, energyAmount, evMinCurrent, evMaxCurrent, evMaxVoltage)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_AC_CHARGING_PARAMETERS_TYPE_H
