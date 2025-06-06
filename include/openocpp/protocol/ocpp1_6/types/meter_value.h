#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_METER_VALUE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_METER_VALUE_H

#include <vector>

#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/sampled_value.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct MeterValue {
        DateTime timestamp;
        std::vector<SampledValue> sampledValue;
        CHARGELAB_JSON_INTRUSIVE(MeterValue, timestamp, sampledValue)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_METER_VALUE_H
