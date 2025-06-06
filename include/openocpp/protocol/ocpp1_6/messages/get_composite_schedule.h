#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_GET_COMPOSITE_SCHEDULE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_GET_COMPOSITE_SCHEDULE_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/charging_rate_unit_type.h"
#include "openocpp/protocol/ocpp1_6/types/get_composite_schedule_status.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/charging_schedule.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct GetCompositeScheduleReq {
        int connectorId;
        int duration;
        std::optional<ChargingRateUnitType> chargingRateUnit;
        CHARGELAB_JSON_INTRUSIVE_CALL(GetCompositeScheduleReq, kGetCompositeSchedule, connectorId, duration, chargingRateUnit)
    };

    struct GetCompositeScheduleRsp {
        GetCompositeScheduleStatus status;
        std::optional<int> connectorId;
        std::optional<DateTime> scheduleStart;
        std::optional<ChargingSchedule> chargingSchedule;
        CHARGELAB_JSON_INTRUSIVE(GetCompositeScheduleRsp, status, connectorId, scheduleStart, chargingSchedule)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_GET_COMPOSITE_SCHEDULE_H
