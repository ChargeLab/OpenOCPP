#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_SCHEDULE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_SCHEDULE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/charging_rate_unit_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/charging_schedule_period_type.h"
#include "openocpp/protocol/ocpp2_0/types/sales_tariff_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ChargingScheduleType {
        int id {};
        std::optional<DateTime> startSchedule {};
        std::optional<int> duration {};
        ChargingRateUnitEnumType chargingRateUnit {};
        std::optional<double> minChargingRate {};
        std::vector<ChargingSchedulePeriodType> chargingSchedulePeriod {};
        std::optional<SalesTariffType> salesTariff {};
        CHARGELAB_JSON_INTRUSIVE(ChargingScheduleType, id, startSchedule, duration, chargingRateUnit, minChargingRate, chargingSchedulePeriod, salesTariff)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CHARGING_SCHEDULE_TYPE_H
