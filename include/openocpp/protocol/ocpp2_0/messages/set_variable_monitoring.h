#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SET_VARIABLE_MONITORING_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SET_VARIABLE_MONITORING_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/set_monitoring_data_type.h"
#include "openocpp/protocol/ocpp2_0/types/set_monitoring_result_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SetVariableMonitoringRequest {
        std::vector<SetMonitoringDataType> setMonitoringData {};
        CHARGELAB_JSON_INTRUSIVE_CALL(SetVariableMonitoringRequest, kSetVariableMonitoring, setMonitoringData)
    };


    struct SetVariableMonitoringResponse {
        std::vector<SetMonitoringResultType> setMonitoringResult {};
        CHARGELAB_JSON_INTRUSIVE(SetVariableMonitoringResponse, setMonitoringResult)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SET_VARIABLE_MONITORING_H
