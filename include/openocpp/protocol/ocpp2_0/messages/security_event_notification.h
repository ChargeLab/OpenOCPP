#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SECURITY_EVENT_NOTIFICATION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SECURITY_EVENT_NOTIFICATION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SecurityEventNotificationRequest {
        StringPrimitive<50> type {};
        DateTime timestamp {};
        std::optional<StringPrimitive<255>> techInfo {};
        CHARGELAB_JSON_INTRUSIVE_CALL(SecurityEventNotificationRequest, kSecurityEventNotification, type, timestamp, techInfo)
    };


    struct SecurityEventNotificationResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(SecurityEventNotificationResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SECURITY_EVENT_NOTIFICATION_H
