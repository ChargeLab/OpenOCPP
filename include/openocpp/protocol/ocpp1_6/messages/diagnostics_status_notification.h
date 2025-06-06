#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_DIAGNOSTICS_STATUS_NOTIFICATION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_DIAGNOSTICS_STATUS_NOTIFICATION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/diagnostics_status.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct DiagnosticsStatusNotificationReq {
        DiagnosticsStatus status;
        CHARGELAB_JSON_INTRUSIVE_CALL(DiagnosticsStatusNotificationReq, kDiagnosticsStatusNotification, status)
    };

    struct DiagnosticsStatusNotificationRsp {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(DiagnosticsStatusNotificationRsp)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_DIAGNOSTICS_STATUS_NOTIFICATION_H
