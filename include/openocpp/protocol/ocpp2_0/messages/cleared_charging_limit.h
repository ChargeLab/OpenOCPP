#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CLEARED_CHARGING_LIMIT_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CLEARED_CHARGING_LIMIT_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/charging_limit_source_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ClearedChargingLimitRequest {
        ChargingLimitSourceEnumType chargingLimitSource {};
        std::optional<int> evseId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(ClearedChargingLimitRequest, kClearedChargingLimit, chargingLimitSource, evseId)
    };


    struct ClearedChargingLimitResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(ClearedChargingLimitResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CLEARED_CHARGING_LIMIT_H
