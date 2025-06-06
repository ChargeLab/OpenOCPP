#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_LOG_STATUS_NOTIFICATION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_LOG_STATUS_NOTIFICATION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/upload_log_status_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct LogStatusNotificationRequest {
        UploadLogStatusEnumType status {};
        std::optional<int> requestId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(LogStatusNotificationRequest, kLogStatusNotification, status, requestId)
    };


    struct LogStatusNotificationResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(LogStatusNotificationResponse)
    };

}


#endif //CHARGELAB_OPEN_FIRMWARE_2_0_LOG_STATUS_NOTIFICATION_H
