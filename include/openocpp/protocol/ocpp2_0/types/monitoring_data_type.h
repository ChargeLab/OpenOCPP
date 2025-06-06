#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_MONITORING_DATA_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_MONITORING_DATA_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/component_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_monitoring_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct MonitoringDataType {
        ComponentType component {};
        VariableType variable {};
        std::vector<VariableMonitoringType> variableMonitoring {};
        CHARGELAB_JSON_INTRUSIVE(MonitoringDataType, component, variable, variableMonitoring)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_MONITORING_DATA_TYPE_H
