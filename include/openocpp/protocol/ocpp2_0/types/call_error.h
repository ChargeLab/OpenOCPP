#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CALL_ERROR_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CALL_ERROR_H

#include "openocpp/protocol/common/raw_json.h"
#include "openocpp/protocol/ocpp2_0/types/error_code.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp2_0 {
    struct CallError {
        ErrorCode code;
        StringPrimitive<255> description;
        common::RawJson details;
        CHARGELAB_JSON_INTRUSIVE(CallError, code, description, details)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CALL_ERROR_H
