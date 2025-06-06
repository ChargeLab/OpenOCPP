#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_LOG_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_LOG_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/log_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/log_parameters_type.h"
#include "openocpp/protocol/ocpp2_0/types/log_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetLogRequest {
        LogEnumType logType {};
        int requestId {};
        std::optional<int> retries {};
        std::optional<int> retryInterval {};
        LogParametersType log {};
        CHARGELAB_JSON_INTRUSIVE_CALL(GetLogRequest, kGetLog, logType, requestId, retries, retryInterval, log)
    };


    struct GetLogResponse {
        LogStatusEnumType status {};
        std::optional<StringPrimitive<255>> filename {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(GetLogResponse, status, filename, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_LOG_H
