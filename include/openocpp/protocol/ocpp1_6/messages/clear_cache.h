#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CLEAR_CACHE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CLEAR_CACHE_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/clear_cache_status.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct ClearCacheReq {
        CHARGELAB_JSON_INTRUSIVE_EMPTY_CALL(ClearCacheReq, kClearCache)
    };

    struct ClearCacheRsp {
        ClearCacheStatus status;
        CHARGELAB_JSON_INTRUSIVE(ClearCacheRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CLEAR_CACHE_H
