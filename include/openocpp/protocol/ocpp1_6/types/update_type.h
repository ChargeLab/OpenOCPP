#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_UPDATE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_UPDATE_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(UpdateType, 
        Differential,
        Full
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_UPDATE_TYPE_H
