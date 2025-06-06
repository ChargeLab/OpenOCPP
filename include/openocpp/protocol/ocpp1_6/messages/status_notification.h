#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_STATUS_NOTIFICATION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_STATUS_NOTIFICATION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/charge_point_error_code.h"
#include "openocpp/protocol/ocpp1_6/types/charge_point_status.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct StatusNotificationReq {
        int connectorId;
        ChargePointErrorCode errorCode;
        std::optional<CiString50Type> info;
        ChargePointStatus status;
        std::optional<DateTime> timestamp;
        std::optional<CiString255Type> vendorId;
        std::optional<CiString50Type> vendorErrorCode;
        CHARGELAB_JSON_INTRUSIVE_CALL(StatusNotificationReq, kStatusNotification, connectorId, errorCode, info, status, timestamp, vendorId, vendorErrorCode)
    };

    struct StatusNotificationRsp {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(StatusNotificationRsp)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_STATUS_NOTIFICATION_H
