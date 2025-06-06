#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_UPDATE_STATUS_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_UPDATE_STATUS_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(UpdateStatus,
        Accepted,
        Failed,
        NotSupported,
        VersionMismatch
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_UPDATE_STATUS_H
