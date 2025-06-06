#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_MONITORING_REPORT_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_MONITORING_REPORT_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/monitoring_data_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct NotifyMonitoringReportRequest {
        int requestId {};
        std::optional<bool> tbc {};
        int seqNo {};
        DateTime generatedAt {};
        std::optional<std::vector<MonitoringDataType>> monitor {};
        CHARGELAB_JSON_INTRUSIVE_CALL(NotifyMonitoringReportRequest, kNotifyMonitoringReport, requestId, tbc, seqNo, generatedAt, monitor)
    };


    struct NotifyMonitoringReportResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(NotifyMonitoringReportResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_MONITORING_REPORT_H
