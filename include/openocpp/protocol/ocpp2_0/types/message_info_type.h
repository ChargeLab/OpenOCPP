#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_INFO_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_INFO_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/message_priority_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/message_state_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/message_content_type.h"
#include "openocpp/protocol/ocpp2_0/types/component_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct MessageInfoType {
        int id {};
        MessagePriorityEnumType priority {};
        std::optional<MessageStateEnumType> state {};
        std::optional<DateTime> startDateTime {};
        std::optional<DateTime> endDateTime {};
        std::optional<IdentifierStringPrimitive<36>> transactionId {};
        MessageContentType message {};
        std::optional<ComponentType> display {};
        CHARGELAB_JSON_INTRUSIVE(MessageInfoType, id, priority, state, startDateTime, endDateTime, transactionId, message, display)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_INFO_TYPE_H
