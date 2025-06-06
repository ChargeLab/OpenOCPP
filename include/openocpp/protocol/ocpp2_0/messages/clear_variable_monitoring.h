#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_VARIABLE_MONITORING_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_VARIABLE_MONITORING_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/clear_monitoring_result_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ClearVariableMonitoringRequest {
        std::vector<int> id {};
        CHARGELAB_JSON_INTRUSIVE_CALL(ClearVariableMonitoringRequest, kClearVariableMonitoring, id)
    };


    struct ClearVariableMonitoringResponse {
        std::vector<ClearMonitoringResultType> clearMonitoringResult {};
        CHARGELAB_JSON_INTRUSIVE(ClearVariableMonitoringResponse, clearMonitoringResult)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_VARIABLE_MONITORING_H
