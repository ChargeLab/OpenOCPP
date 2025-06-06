#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_ENERGY_TRANSFER_MODE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_ENERGY_TRANSFER_MODE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(EnergyTransferModeEnumType, 
        DC,
        AC_single_phase,
        AC_two_phase,
        AC_three_phase
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_ENERGY_TRANSFER_MODE_ENUM_TYPE_H
