#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_COMPONENT_CRITERION_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_COMPONENT_CRITERION_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(ComponentCriterionEnumType, 
        Active,
        Available,
        Enabled,
        Problem
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_COMPONENT_CRITERION_ENUM_TYPE_H
