#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_METER_VALUES_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_METER_VALUES_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/meter_value.h"
#include "openocpp/protocol/common/vector.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct MeterValuesReq {
        int connectorId;
        std::optional<int> transactionId;
        std::vector<MeterValue> meterValue;
        CHARGELAB_JSON_INTRUSIVE_CALL(MeterValuesReq, kMeterValues, connectorId, transactionId, meterValue)
    };

    struct MeterValuesRsp {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(MeterValuesRsp)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_METER_VALUES_H
