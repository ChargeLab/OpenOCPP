#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_REPORT_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_REPORT_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/report_data_type.h"
#include "openocpp/protocol/common/vector.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct NotifyReportRequest {
        int requestId {};
        DateTime generatedAt {};
        std::optional<bool> tbc {};
        int seqNo {};
        Vector<ReportDataType, true> reportData {};
        CHARGELAB_JSON_INTRUSIVE_CALL(NotifyReportRequest, kNotifyReport, requestId, generatedAt, tbc, seqNo, reportData)
    };


    struct NotifyReportResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(NotifyReportResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_REPORT_H
