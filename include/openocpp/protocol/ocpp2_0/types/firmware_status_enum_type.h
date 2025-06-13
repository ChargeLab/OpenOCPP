#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_FIRMWARE_STATUS_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_FIRMWARE_STATUS_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(FirmwareStatusEnumType, 
        Downloaded,
        DownloadFailed,
        Downloading,
        DownloadScheduled,
        DownloadPaused,
        Idle,
        FirmwareStatusNotificationRequest,
        InstallationFailed,
        Installing,
        Installed,
        InstallRebooting,
        InstallScheduled,
        InstallVerificationFailed,
        InvalidSignature,
        SignatureVerified
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_FIRMWARE_STATUS_ENUM_TYPE_H
