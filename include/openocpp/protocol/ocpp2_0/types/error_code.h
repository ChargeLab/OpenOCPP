#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_ERROR_CODE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_ERROR_CODE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM( ErrorCode, 
        FormatViolation,
        GenericError,
        InternalError,
        MessageTypeNotSupported,
        NotImplemented,
        NotSupported,
        OccurrenceConstraintViolation,
        PropertyConstraintViolation,
        ProtocolError,
        RpcFrameworkError,
        SecurityError,
        TypeConstraintViolation
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_ERROR_CODE_H
