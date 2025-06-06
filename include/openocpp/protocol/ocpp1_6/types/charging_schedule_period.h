#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_SCHEDULE_PERIOD_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_SCHEDULE_PERIOD_H

#include <string>

#include "openocpp/protocol/ocpp1_6/types/authorization_status.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct ChargingSchedulePeriod {
        std::int64_t startPeriod;
        double limit;
        std::optional<int> numberPhases {1};
        CHARGELAB_JSON_INTRUSIVE(ChargingSchedulePeriod, startPeriod, limit, numberPhases)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_SCHEDULE_PERIOD_H
