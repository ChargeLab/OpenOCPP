#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_UPDATE_FIRMWARE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_UPDATE_FIRMWARE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/firmware_type.h"
#include "openocpp/protocol/ocpp2_0/types/update_firmware_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct UpdateFirmwareRequest {
        std::optional<int> retries {};
        std::optional<int> retryInterval {};
        int requestId {};
        FirmwareType firmware {};
        CHARGELAB_JSON_INTRUSIVE_CALL(UpdateFirmwareRequest, kUpdateFirmware, retries, retryInterval, requestId, firmware)
    };


    struct UpdateFirmwareResponse {
        UpdateFirmwareStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(UpdateFirmwareResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_UPDATE_FIRMWARE_H
