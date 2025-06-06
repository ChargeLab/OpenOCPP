#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_PUBLISH_FIRMWARE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_PUBLISH_FIRMWARE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/generic_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct PublishFirmwareRequest {
        StringPrimitive<512> location {};
        std::optional<int> retries {};
        IdentifierStringPrimitive<32> checksum {};
        int requestId {};
        std::optional<int> retryInterval {};
        CHARGELAB_JSON_INTRUSIVE_CALL(PublishFirmwareRequest, kPublishFirmware, location, retries, checksum, requestId, retryInterval)
    };


    struct PublishFirmwareResponse {
        GenericStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(PublishFirmwareResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_PUBLISH_FIRMWARE_H
