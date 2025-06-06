#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_STATUS_NOTIFICATION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_STATUS_NOTIFICATION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/connector_status_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct StatusNotificationRequest {
        DateTime timestamp {};
        ConnectorStatusEnumType connectorStatus {};
        int evseId {};
        int connectorId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(StatusNotificationRequest, kStatusNotification, timestamp, connectorStatus, evseId, connectorId)
    };


    struct StatusNotificationResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(StatusNotificationResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_STATUS_NOTIFICATION_H
