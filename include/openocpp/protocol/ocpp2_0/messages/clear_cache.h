#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_CACHE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_CACHE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/clear_cache_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ClearCacheRequest {
        CHARGELAB_JSON_INTRUSIVE_EMPTY_CALL(ClearCacheRequest, kClearCache)
    };


    struct ClearCacheResponse {
        ClearCacheStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(ClearCacheResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CLEAR_CACHE_H
