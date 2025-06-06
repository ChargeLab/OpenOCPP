#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SET_DISPLAY_MESSAGE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SET_DISPLAY_MESSAGE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/message_info_type.h"
#include "openocpp/protocol/ocpp2_0/types/display_message_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SetDisplayMessageRequest {
        MessageInfoType message {};
        CHARGELAB_JSON_INTRUSIVE_CALL(SetDisplayMessageRequest, kSetDisplayMessage, message)
    };


    struct SetDisplayMessageResponse {
        DisplayMessageStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(SetDisplayMessageResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SET_DISPLAY_MESSAGE_H
