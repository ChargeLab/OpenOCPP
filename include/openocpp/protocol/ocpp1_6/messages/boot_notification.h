#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_BOOT_NOTIFICATION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_BOOT_NOTIFICATION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/registration_status.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp1_6 {
    struct BootNotificationReq {
        std::optional<CiString25Type> chargeBoxSerialNumber = std::nullopt;
        CiString20Type chargePointModel {};
        std::optional<CiString25Type> chargePointSerialNumber = std::nullopt;
        CiString20Type chargePointVendor {};
        std::optional<CiString50Type> firmwareVersion = std::nullopt;
        std::optional<CiString20Type> iccid = std::nullopt;
        std::optional<CiString20Type> imsi = std::nullopt;
        std::optional<CiString25Type> meterSerialNumber = std::nullopt;
        std::optional<CiString25Type> meterType = std::nullopt;
        CHARGELAB_JSON_INTRUSIVE_CALL(BootNotificationReq, kBootNotification, chargeBoxSerialNumber, chargePointModel, chargePointSerialNumber, chargePointVendor, firmwareVersion, iccid, imsi, meterSerialNumber, meterType)
    };

    struct BootNotificationRsp {
        DateTime currentTime;
        int interval;
        RegistrationStatus status;
        CHARGELAB_JSON_INTRUSIVE(BootNotificationRsp, currentTime, interval, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_BOOT_NOTIFICATION_H
