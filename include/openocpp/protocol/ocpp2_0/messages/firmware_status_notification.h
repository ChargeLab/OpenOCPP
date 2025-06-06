#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_FIRMWARE_STATUS_NOTIFICATION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_FIRMWARE_STATUS_NOTIFICATION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/firmware_status_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct FirmwareStatusNotificationRequest {
        FirmwareStatusEnumType status {};
        std::optional<int> requestId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(FirmwareStatusNotificationRequest, kFirmwareStatusNotification, status, requestId)
    };


    struct FirmwareStatusNotificationResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(FirmwareStatusNotificationResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_FIRMWARE_STATUS_NOTIFICATION_H
