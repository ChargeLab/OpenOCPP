#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_ATTRIBUTE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_ATTRIBUTE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(AttributeEnumType, 
        Actual,
        Target,
        MinSet,
        MaxSet,
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_ATTRIBUTE_ENUM_TYPE_H
