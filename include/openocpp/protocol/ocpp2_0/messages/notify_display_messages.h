#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_DISPLAY_MESSAGES_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_DISPLAY_MESSAGES_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/message_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct NotifyDisplayMessagesRequest {
        int requestId {};
        std::optional<bool> tbc {};
        std::optional<std::vector<MessageInfoType>> messageInfo {};
        CHARGELAB_JSON_INTRUSIVE_CALL(NotifyDisplayMessagesRequest, kNotifyDisplayMessages, requestId, tbc, messageInfo)
    };


    struct NotifyDisplayMessagesResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(NotifyDisplayMessagesResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_DISPLAY_MESSAGES_H
