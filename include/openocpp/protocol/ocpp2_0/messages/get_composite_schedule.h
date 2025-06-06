#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_COMPOSITE_SCHEDULE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_COMPOSITE_SCHEDULE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/charging_rate_unit_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/generic_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/composite_schedule_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetCompositeScheduleRequest {
        int duration {};
        std::optional<ChargingRateUnitEnumType> chargingRateUnit {};
        int evseId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(GetCompositeScheduleRequest, kGetCompositeSchedule, duration, chargingRateUnit, evseId)
    };


    struct GetCompositeScheduleResponse {
        GenericStatusEnumType status {};
        std::optional<CompositeScheduleType> schedule {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(GetCompositeScheduleResponse, status, schedule, statusInfo)
    };


}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_COMPOSITE_SCHEDULE_H
