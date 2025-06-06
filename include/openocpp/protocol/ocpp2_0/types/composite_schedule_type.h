#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_COMPONENT_SCHEDULE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_COMPONENT_SCHEDULE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/charging_rate_unit_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/charging_schedule_period_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct CompositeScheduleType {
        int evseId {};
        int duration {};
        DateTime scheduleStart {};
        ChargingRateUnitEnumType chargingRateUnit {};
        std::vector<ChargingSchedulePeriodType> chargingSchedulePeriod {};
        CHARGELAB_JSON_INTRUSIVE(CompositeScheduleType, evseId, duration, scheduleStart, chargingRateUnit, chargingSchedulePeriod)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_COMPONENT_SCHEDULE_TYPE_H
