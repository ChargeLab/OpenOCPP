#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_BOOT_NOTIFICATION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_BOOT_NOTIFICATION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/boot_reason_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/charging_station_type.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/registration_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct BootNotificationRequest {
        BootReasonEnumType reason {};
        ChargingStationType chargingStation {};
        CHARGELAB_JSON_INTRUSIVE_CALL(BootNotificationRequest, kBootNotification, reason, chargingStation)
    };


    struct BootNotificationResponse {
        DateTime currentTime {};
        int interval {};
        RegistrationStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(BootNotificationResponse, currentTime, interval, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_BOOT_NOTIFICATION_H
