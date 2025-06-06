#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CALL_ERROR_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CALL_ERROR_H

#include "openocpp/protocol/ocpp1_6/types/error_code.h"
#include "openocpp/protocol/common/raw_json.h"
#include "openocpp/helpers/json.h"

#include <string>

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct CallError {
        ErrorCode code;
        std::string description;
        common::RawJson details;
        CHARGELAB_JSON_INTRUSIVE(CallError, code, description, details)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CALL_ERROR_H
