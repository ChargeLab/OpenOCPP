#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CHANGE_CONFIGURATION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CHANGE_CONFIGURATION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/protocol/ocpp1_6/types/configuration_status.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct ChangeConfigurationReq {
        CiString50Type key;
        CiString500Type value;
        CHARGELAB_JSON_INTRUSIVE_CALL(ChangeConfigurationReq, kChangeConfiguration, key, value)
    };

    struct ChangeConfigurationRsp {
        ConfigurationStatus status;
        CHARGELAB_JSON_INTRUSIVE(ChangeConfigurationRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CHANGE_CONFIGURATION_H
