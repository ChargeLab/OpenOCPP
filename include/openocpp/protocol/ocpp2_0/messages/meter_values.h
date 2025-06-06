#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_METER_VALUES_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_METER_VALUES_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/meter_value_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct MeterValuesRequest {
        int evseId {};
        std::vector<MeterValueType> meterValue {};
        CHARGELAB_JSON_INTRUSIVE_CALL(MeterValuesRequest, kMeterValues, evseId, meterValue)
    };


    struct MeterValuesResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(MeterValuesResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_METER_VALUES_H
