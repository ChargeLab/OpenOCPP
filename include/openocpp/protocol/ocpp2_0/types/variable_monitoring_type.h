#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_MONITORING_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_MONITORING_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/monitor_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct VariableMonitoringType {
        int id {};
        bool transaction {};
        double value {};
        MonitorEnumType type {};
        int severity {};
        CHARGELAB_JSON_INTRUSIVE(VariableMonitoringType, id, transaction, value, type, severity)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_VARIABLE_MONITORING_TYPE_H
