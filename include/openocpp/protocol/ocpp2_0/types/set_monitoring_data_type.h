#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_DATA_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_DATA_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/monitor_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/component_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SetMonitoringDataType {
        std::optional<int> id {};
        std::optional<bool> transaction {};
        double value {};
        MonitorEnumType type {};
        int severity {};
        ComponentType component {};
        VariableType variable {};
        CHARGELAB_JSON_INTRUSIVE(SetMonitoringDataType, id, transaction, value, type, severity, component, variable)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_DATA_TYPE_H
