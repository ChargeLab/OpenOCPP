#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_FIRMWARE_STATUS_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_FIRMWARE_STATUS_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(FirmwareStatus, 
        Downloaded,
        DownloadFailed,
        Downloading,
        Idle,
        InstallationFailed,
        Installing,
        Installed
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_FIRMWARE_STATUS_H
