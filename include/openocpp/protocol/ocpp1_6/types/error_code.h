#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_ERROR_CODE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_ERROR_CODE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(ErrorCode,
        NotImplemented,
        NotSupported,
        InternalError,
        ProtocolError,
        SecurityError,
        FormationViolation,
        PropertyConstraintViolation,
        OccurenceConstraintViolation,
        TypeConstraintViolation,
        GenericError
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_ERROR_CODE_H
