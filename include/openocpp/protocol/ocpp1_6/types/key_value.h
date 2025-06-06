#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_KEY_VALUE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_KEY_VALUE_H

#include <string>

#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct KeyValue {
        CiString50Type key;
        bool readonly;
        std::optional<CiString500Type> value;
        CHARGELAB_JSON_INTRUSIVE(KeyValue, key, readonly, value)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_KEY_VALUE_H
