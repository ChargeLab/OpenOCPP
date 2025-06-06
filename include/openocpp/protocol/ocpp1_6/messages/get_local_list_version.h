#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_GET_LOCAL_LIST_VERSION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_GET_LOCAL_LIST_VERSION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct GetLocalListVersionReq {
        CHARGELAB_JSON_INTRUSIVE_EMPTY_CALL(GetLocalListVersionReq, kGetLocalListVersion)
    };

    struct GetLocalListVersionRsp {
        int listVersion;
        CHARGELAB_JSON_INTRUSIVE(GetLocalListVersionRsp, listVersion)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_GET_LOCAL_LIST_VERSION_H
