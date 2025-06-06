#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_EVENT_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_EVENT_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/event_data_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct NotifyEventRequest {
        DateTime generatedAt {};
        std::optional<bool> tbc {};
        int seqNo {};
        std::vector<EventDataType> eventData {};
        CHARGELAB_JSON_INTRUSIVE_CALL(NotifyEventRequest, kNotifyEvent, generatedAt, tbc, seqNo, eventData)
    };


    struct NotifyEventResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(NotifyEventResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_NOTIFY_EVENT_H
