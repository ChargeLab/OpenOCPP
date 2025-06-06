#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_GET_DIAGNOSTICS_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_GET_DIAGNOSTICS_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/any_uri.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct GetDiagnosticsReq {
        AnyUri location;
        std::optional<int> retries;
        std::optional<int> retryInterval;
        std::optional<DateTime> startTime;
        std::optional<DateTime> stopTime;
        CHARGELAB_JSON_INTRUSIVE_CALL(GetDiagnosticsReq, kGetDiagnostics, location, retries, retryInterval, startTime, stopTime)
    };

    struct GetDiagnosticsRsp {
        std::optional<std::string> fileName;
        CHARGELAB_JSON_INTRUSIVE(GetDiagnosticsRsp, fileName)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_GET_DIAGNOSTICS_H
