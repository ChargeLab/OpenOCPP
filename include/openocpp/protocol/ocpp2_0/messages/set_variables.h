#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SET_VARIABLES_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SET_VARIABLES_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/set_variable_data_type.h"
#include "openocpp/protocol/ocpp2_0/types/set_variable_result_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SetVariablesRequest {
        std::vector<SetVariableDataType> setVariableData {};
        CHARGELAB_JSON_INTRUSIVE_CALL(SetVariablesRequest, kSetVariables, setVariableData)
    };


    struct SetVariablesResponse {
        std::vector<SetVariableResultType> setVariableResult {};
        CHARGELAB_JSON_INTRUSIVE(SetVariablesResponse, setVariableResult)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SET_VARIABLES_H
