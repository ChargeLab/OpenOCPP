#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_SCHEDULE_PERIOD_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_SCHEDULE_PERIOD_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ChargingSchedulePeriodType {
        int startPeriod {};
        double limit {};
        std::optional<int> numberPhases {};
        std::optional<int> phaseToUse {};
        CHARGELAB_JSON_INTRUSIVE(ChargingSchedulePeriodType, startPeriod, limit, numberPhases, phaseToUse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_SCHEDULE_PERIOD_TYPE_H
