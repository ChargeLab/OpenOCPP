#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_RESET_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_RESET_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/reset_type.h"
#include "openocpp/protocol/ocpp1_6/types/reset_status.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct ResetReq {
        ResetType type;
        CHARGELAB_JSON_INTRUSIVE_CALL(ResetReq, kReset, type)
    };

    struct ResetRsp {
        ResetStatus status;
        CHARGELAB_JSON_INTRUSIVE(ResetRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_RESET_H
