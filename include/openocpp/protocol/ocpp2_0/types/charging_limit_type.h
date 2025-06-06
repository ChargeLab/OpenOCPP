#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_LIMIT_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_LIMIT_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/charging_limit_source_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ChargingLimitType {
        ChargingLimitSourceEnumType chargingLimitSource {};
        std::optional<bool> isGridCritical {};
        CHARGELAB_JSON_INTRUSIVE(ChargingLimitType, chargingLimitSource, isGridCritical)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_LIMIT_TYPE_H
