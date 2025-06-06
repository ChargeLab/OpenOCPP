#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_UPDATE_FIRMWARE_STATUS_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_UPDATE_FIRMWARE_STATUS_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(UpdateFirmwareStatusEnumType, 
        Accepted,
        Rejected,
        AcceptedCanceled,
        InvalidCertificate,
        RevokedCertificate
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_UPDATE_FIRMWARE_STATUS_ENUM_TYPE_H
