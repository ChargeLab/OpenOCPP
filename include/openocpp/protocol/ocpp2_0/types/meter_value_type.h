#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_METER_VALUE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_METER_VALUE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/sampled_value_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct MeterValueType {
        DateTime timestamp {};
        std::vector<SampledValueType> sampledValue {};
        CHARGELAB_JSON_INTRUSIVE(MeterValueType, timestamp, sampledValue)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_METER_VALUE_TYPE_H
