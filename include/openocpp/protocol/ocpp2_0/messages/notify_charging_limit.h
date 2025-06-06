#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_CHARGING_LIMIT_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_CHARGING_LIMIT_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/charging_limit_type.h"
#include "openocpp/protocol/ocpp2_0/types/charging_schedule_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct NotifyChargingLimitRequest {
        std::optional<int> evseId {};
        ChargingLimitType chargingLimit {};
        std::optional<std::vector<ChargingScheduleType>> chargingSchedule {};
        CHARGELAB_JSON_INTRUSIVE_CALL(NotifyChargingLimitRequest, kNotifyChargingLimit, evseId, chargingLimit, chargingSchedule)
    };


    struct NotifyChargingLimitResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(NotifyChargingLimitResponse)
    };

}


#endif //CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_CHARGING_LIMIT_H
