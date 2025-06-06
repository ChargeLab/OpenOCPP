#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_UPLOAD_LOG_STATUS_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_UPLOAD_LOG_STATUS_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(UploadLogStatusEnumType, 
        BadMessage,
        Idle,
        NotSupportedOperation,
        PermissionDenied,
        Uploaded,
        UploadFailure,
        Uploading,
        AcceptedCanceled
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_UPLOAD_LOG_STATUS_ENUM_TYPE_H
