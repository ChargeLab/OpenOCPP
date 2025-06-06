#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_LOCAL_LIST_VERSION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_LOCAL_LIST_VERSION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetLocalListVersionRequest {
        CHARGELAB_JSON_INTRUSIVE_EMPTY_CALL(GetLocalListVersionRequest, kGetLocalListVersion)
    };


    struct GetLocalListVersionResponse {
        int versionNumber {};
        CHARGELAB_JSON_INTRUSIVE(GetLocalListVersionResponse, versionNumber)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_LOCAL_LIST_VERSION_H
