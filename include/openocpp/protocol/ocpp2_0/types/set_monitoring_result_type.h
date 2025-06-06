#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_RESULT_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_RESULT_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/set_monitoring_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/monitor_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/component_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SetMonitoringResultType {
        std::optional<int> id {};
        SetMonitoringStatusEnumType status {};
        MonitorEnumType type {};
        int severity {};
        ComponentType component {};
        VariableType variable {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(SetMonitoringResultType, id, status, type, severity, component, variable, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_RESULT_TYPE_H
