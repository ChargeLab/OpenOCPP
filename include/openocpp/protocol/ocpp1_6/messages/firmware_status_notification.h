#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_FIRMWARE_STATUS_NOTIFICATION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_FIRMWARE_STATUS_NOTIFICATION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/firmware_status.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct FirmwareStatusNotificationReq {
        FirmwareStatus status;
        CHARGELAB_JSON_INTRUSIVE_CALL(FirmwareStatusNotificationReq, kFirmwareStatusNotification, status)
    };

    struct FirmwareStatusNotificationRsp {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(FirmwareStatusNotificationRsp)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_FIRMWARE_STATUS_NOTIFICATION_H
