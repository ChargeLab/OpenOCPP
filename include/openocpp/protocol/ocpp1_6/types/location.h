#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_LOCATION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_LOCATION_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(Location, 
        Body,
        Cable,
        EV,
        Inlet,
        Outlet
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_LOCATION_H
