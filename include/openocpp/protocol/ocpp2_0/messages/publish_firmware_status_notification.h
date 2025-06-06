#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_PUBLISH_FIRMWARE_STATUS_NOTIFICATION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_PUBLISH_FIRMWARE_STATUS_NOTIFICATION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/publish_firmware_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct PublishFirmwareStatusNotificationRequest {
        PublishFirmwareStatusEnumType status {};
        std::optional<std::vector<StringPrimitive<512>>> location {};
        std::optional<int> requestId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(PublishFirmwareStatusNotificationRequest, kPublishFirmwareStatusNotification, status, location, requestId)
    };


    struct PublishFirmwareStatusNotificationResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(PublishFirmwareStatusNotificationResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_PUBLISH_FIRMWARE_STATUS_NOTIFICATION_H
