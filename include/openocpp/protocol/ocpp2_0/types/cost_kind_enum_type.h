#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_COST_KIND_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_COST_KIND_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(CostKindEnumType, 
        CarbonDioxideEmission,
        RelativePricePercentage,
        RenewableGenerationPercentage
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_COST_KIND_ENUM_TYPE_H
