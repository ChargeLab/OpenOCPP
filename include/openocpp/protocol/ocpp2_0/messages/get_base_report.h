#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_BASE_REPORT_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_BASE_REPORT_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/report_base_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/generic_device_model_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetBaseReportRequest {
        int requestId {};
        ReportBaseEnumType reportBase {};
        CHARGELAB_JSON_INTRUSIVE_CALL(GetBaseReportRequest, kGetBaseReport, requestId, reportBase)
    };


    struct GetBaseReportResponse {
        GenericDeviceModelStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(GetBaseReportResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_BASE_REPORT_H
