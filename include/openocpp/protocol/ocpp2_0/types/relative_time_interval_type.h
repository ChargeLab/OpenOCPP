#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_RELATIVE_TIME_INTERVAL_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_RELATIVE_TIME_INTERVAL_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct RelativeTimeIntervalType {
        int start {};
        std::optional<int> duration {};
        CHARGELAB_JSON_INTRUSIVE(RelativeTimeIntervalType, start, duration)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_RELATIVE_TIME_INTERVAL_TYPE_H
