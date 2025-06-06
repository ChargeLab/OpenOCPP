#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_LOG_PARAMETERS_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_LOG_PARAMETERS_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct LogParametersType {
        StringPrimitive<512> remoteLocation {};
        std::optional<DateTime> oldestTimestamp {};
        std::optional<DateTime> latestTimestamp {};
        CHARGELAB_JSON_INTRUSIVE(LogParametersType, remoteLocation, oldestTimestamp, latestTimestamp)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_LOG_PARAMETERS_TYPE_H
