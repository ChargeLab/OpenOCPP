#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_MONITORING_RESULT_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_MONITORING_RESULT_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/clear_monitoring_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ClearMonitoringResultType {
        ClearMonitoringStatusEnumType status {};
        int id {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(ClearMonitoringResultType, status, id, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_MONITORING_RESULT_TYPE_H
