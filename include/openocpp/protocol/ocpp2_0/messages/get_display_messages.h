#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_DISPLAY_MESSAGES_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_DISPLAY_MESSAGES_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/message_priority_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/message_state_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/get_display_messages_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetDisplayMessagesRequest {
        std::optional<std::vector<int>> id {};
        int requestId {};
        std::optional<MessagePriorityEnumType> priority {};
        std::optional<MessageStateEnumType> state {};
        CHARGELAB_JSON_INTRUSIVE_CALL(GetDisplayMessagesRequest, kGetDisplayMessages, id, requestId, priority, state)
    };


    struct GetDisplayMessagesResponse {
        GetDisplayMessagesStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(GetDisplayMessagesResponse, status, statusInfo)
    };


}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_DISPLAY_MESSAGES_H
