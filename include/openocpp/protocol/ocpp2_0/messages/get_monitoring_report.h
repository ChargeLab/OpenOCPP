#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_MONITORING_REPORT_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_MONITORING_REPORT_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/monitoring_criterion_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/component_variable_type.h"
#include "openocpp/protocol/ocpp2_0/types/generic_device_model_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetMonitoringReportRequest {
        int requestId {};
        std::optional<std::vector<MonitoringCriterionEnumType>> monitoringCriteria {};
        std::optional<std::vector<ComponentVariableType>> componentVariable {};
        CHARGELAB_JSON_INTRUSIVE_CALL(GetMonitoringReportRequest, kGetMonitoringReport, requestId, monitoringCriteria, componentVariable)
    };


    struct GetMonitoringReportResponse {
        GenericDeviceModelStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(GetMonitoringReportResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_MONITORING_REPORT_H
