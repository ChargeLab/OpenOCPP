#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_VARIABLES_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_VARIABLES_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/get_variable_data_type.h"
#include "openocpp/protocol/ocpp2_0/types/get_variable_result_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetVariablesRequest {
        std::vector<GetVariableDataType> getVariableData {};
        CHARGELAB_JSON_INTRUSIVE_CALL(GetVariablesRequest, kGetVariables, getVariableData)
    };


    struct GetVariablesResponse {
        Vector<GetVariableResultType, false> getVariableResult {};
        CHARGELAB_JSON_INTRUSIVE(GetVariablesResponse, getVariableResult)
    };


}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_VARIABLES_H
