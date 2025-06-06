#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_REPORT_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_REPORT_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/component_criterion_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/component_variable_type.h"
#include "openocpp/protocol/ocpp2_0/types/generic_device_model_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetReportRequest {
        int requestId {};
        std::optional<std::vector<ComponentCriterionEnumType>> componentCriteria {};
        std::optional<std::vector<ComponentVariableType>> componentVariable {};
        CHARGELAB_JSON_INTRUSIVE_CALL(GetReportRequest, kGetReport, requestId, componentCriteria, componentVariable)
    };


    struct GetReportResponse {
        GenericDeviceModelStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(GetReportResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_REPORT_H
