#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_VALUE_FORMAT_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_VALUE_FORMAT_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(ValueFormat, 
        Raw,
        SignedData
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_VALUE_FORMAT_H
