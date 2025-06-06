#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_EV_CHARGING_SCHEDULE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_EV_CHARGING_SCHEDULE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/charging_schedule_type.h"
#include "openocpp/protocol/ocpp2_0/types/generic_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct NotifyEVChargingScheduleRequest {
        DateTime timeBase {};
        int evseId {};
        ChargingScheduleType chargingSchedule {};
        CHARGELAB_JSON_INTRUSIVE_CALL(NotifyEVChargingScheduleRequest, kNotifyEVChargingSchedule, timeBase, evseId, chargingSchedule)
    };


    struct NotifyEVChargingScheduleResponse {
        GenericStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(NotifyEVChargingScheduleResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_EV_CHARGING_SCHEDULE_H
