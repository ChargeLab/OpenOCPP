#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_DISPLAY_MESSAGE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_DISPLAY_MESSAGE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/clear_message_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ClearDisplayMessageRequest {
        int id {};
        CHARGELAB_JSON_INTRUSIVE_CALL(ClearDisplayMessageRequest, kClearDisplayMessage, id)
    };


    struct ClearDisplayMessageResponse {
        ClearMessageStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(ClearDisplayMessageResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_DISPLAY_MESSAGE_H
