#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_NEEDS_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_NEEDS_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/energy_transfer_mode_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/ac_charging_parameters_type.h"
#include "openocpp/protocol/ocpp2_0/types/dc_charging_parameters_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ChargingNeedsType {
        EnergyTransferModeEnumType requestedEnergyTransfer {};
        std::optional<DateTime> departureTime {};
        std::optional<ACChargingParametersType> acChargingParameters {};
        std::optional<DCChargingParametersType> dcChargingParameters {};
        CHARGELAB_JSON_INTRUSIVE(ChargingNeedsType, requestedEnergyTransfer, departureTime, acChargingParameters, dcChargingParameters)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_NEEDS_TYPE_H
