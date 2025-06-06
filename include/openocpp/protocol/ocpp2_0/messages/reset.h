#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_RESET_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_RESET_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/reset_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/reset_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ResetRequest {
        ResetEnumType type {};
        std::optional<int> evseId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(ResetRequest, kReset, type, evseId)
    };


    struct ResetResponse {
        ResetStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(ResetResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_RESET_H
