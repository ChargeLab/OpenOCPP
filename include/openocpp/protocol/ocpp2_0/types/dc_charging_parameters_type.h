#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_DC_CHARGING_PARAMETERS_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_DC_CHARGING_PARAMETERS_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct DCChargingParametersType {
        int evMaxCurrent {};
        int evMaxVoltage {};
        std::optional<int> energyAmount {};
        std::optional<int> evMaxPower {};
        std::optional<int> stateOfCharge {};
        std::optional<int> evEnergyCapacity {};
        std::optional<int> fullSoC {};
        std::optional<int> bulkSoC {};
        CHARGELAB_JSON_INTRUSIVE(DCChargingParametersType, evMaxCurrent, evMaxVoltage, energyAmount, evMaxPower, stateOfCharge, evEnergyCapacity, fullSoC, bulkSoC)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_DC_CHARGING_PARAMETERS_TYPE_H
