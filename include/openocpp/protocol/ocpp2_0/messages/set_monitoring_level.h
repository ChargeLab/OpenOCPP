#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_LEVEL_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_LEVEL_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/generic_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SetMonitoringLevelRequest {
        int severity {};
        CHARGELAB_JSON_INTRUSIVE_CALL(SetMonitoringLevelRequest, kSetMonitoringLevel, severity)
    };


    struct SetMonitoringLevelResponse {
        GenericStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(SetMonitoringLevelResponse, status, statusInfo)
    };


}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_LEVEL_H
