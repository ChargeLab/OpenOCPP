#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_TRIGGER_MESSAGE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_TRIGGER_MESSAGE_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/message_trigger.h"
#include "openocpp/protocol/ocpp1_6/types/trigger_message_status.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct TriggerMessageReq {
        MessageTrigger requestedMessage;
        std::optional<int> connectorId;
        CHARGELAB_JSON_INTRUSIVE_CALL(TriggerMessageReq, kTriggerMessage, requestedMessage, connectorId)
    };

    struct TriggerMessageRsp {
        TriggerMessageStatus status;
        CHARGELAB_JSON_INTRUSIVE(TriggerMessageRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_TRIGGER_MESSAGE_H
