#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_TRIGGER_MESSAGE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_TRIGGER_MESSAGE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/message_trigger_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/evse_type.h"
#include "openocpp/protocol/ocpp2_0/types/trigger_message_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct TriggerMessageRequest {
        MessageTriggerEnumType requestedMessage {};
        std::optional<EVSEType> evse {};
        CHARGELAB_JSON_INTRUSIVE_CALL(TriggerMessageRequest, kTriggerMessage, requestedMessage, evse)
    };


    struct TriggerMessageResponse {
        TriggerMessageStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(TriggerMessageResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_TRIGGER_MESSAGE_H
