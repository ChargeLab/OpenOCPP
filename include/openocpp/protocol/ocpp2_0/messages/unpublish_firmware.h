#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_UNPUBLISH_FIRMWARE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_UNPUBLISH_FIRMWARE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/unpublish_firmware_status_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct UnpublishFirmwareRequest {
        IdentifierStringPrimitive<32> checksum {};
        CHARGELAB_JSON_INTRUSIVE_CALL(UnpublishFirmwareRequest, kUnpublishFirmware, checksum)
    };


    struct UnpublishFirmwareResponse {
        UnpublishFirmwareStatusEnumType status {};
        CHARGELAB_JSON_INTRUSIVE(UnpublishFirmwareResponse, status)
    };


}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_UNPUBLISH_FIRMWARE_H
