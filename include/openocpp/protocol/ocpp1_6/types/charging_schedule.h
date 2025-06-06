#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_SCHEDULE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_SCHEDULE_H

#include <string>
#include <vector>
#include <optional>

#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/charging_rate_unit_type.h"
#include "openocpp/protocol/ocpp1_6/types/charging_schedule_period.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct ChargingSchedule {
        std::optional<int> duration;
        std::optional<DateTime> startSchedule;
        ChargingRateUnitType chargingRateUnit;
        std::vector<ChargingSchedulePeriod> chargingSchedulePeriod;
        std::optional<double> minChargingRate;
        CHARGELAB_JSON_INTRUSIVE(ChargingSchedule, duration, startSchedule, chargingRateUnit, chargingSchedulePeriod, minChargingRate)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CHARGING_SCHEDULE_H
