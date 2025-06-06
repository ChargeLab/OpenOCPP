#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_DATA_TRANSFER_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_DATA_TRANSFER_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/data_transfer_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/protocol/common/raw_json.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct DataTransferRequest {
        std::optional<StringPrimitive<50>> messageId {};
        std::optional<common::RawJson> data {};
        StringPrimitive<255> vendorId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(DataTransferRequest, kDataTransfer, messageId, data, vendorId)
    };


    struct DataTransferResponse {
        DataTransferStatusEnumType status {};
        std::optional<common::RawJson> data {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(DataTransferResponse, status, data, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_DATA_TRANSFER_H
