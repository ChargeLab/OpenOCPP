#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_GET_CONFIGURATION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_GET_CONFIGURATION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/protocol/ocpp1_6/types/key_value.h"
#include "openocpp/protocol/common/vector.h"
#include "openocpp/helpers/json.h"

#include <optional>
#include <vector>

namespace chargelab::ocpp1_6 {
    struct GetConfigurationReq {
        Vector<CiString50Type, true> key;
        CHARGELAB_JSON_INTRUSIVE_CALL(GetConfigurationReq, kGetConfiguration, key)
    };

    struct GetConfigurationRsp {
        Vector<KeyValue, true> configurationKey;
        Vector<CiString50Type, true> unknownKey;
        CHARGELAB_JSON_INTRUSIVE(GetConfigurationRsp, configurationKey, unknownKey)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_GET_CONFIGURATION_H
