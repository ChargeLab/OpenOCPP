#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_UPDATE_FIRMWARE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_UPDATE_FIRMWARE_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/any_uri.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    struct UpdateFirmwareReq {
        AnyUri location;
        std::optional<int> retries;
        DateTime retrieveDate;
        std::optional<int> retryInterval;
        CHARGELAB_JSON_INTRUSIVE_CALL(UpdateFirmwareReq, kUpdateFirmware, location, retries, retrieveDate, retryInterval)
    };

    struct UpdateFirmwareRsp {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(UpdateFirmwareRsp)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_UPDATE_FIRMWARE_H
