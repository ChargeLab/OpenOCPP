#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_DATA_TRANSFER_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_DATA_TRANSFER_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/protocol/ocpp1_6/types/data_transfer_status.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct DataTransferReq {
        CiString255Type vendorId;
        std::optional<CiString50Type> messageId;
        std::optional<std::string> data;
        CHARGELAB_JSON_INTRUSIVE_CALL(DataTransferReq, kDataTransfer, vendorId, messageId, data)
    };

    struct DataTransferRsp {
        DataTransferStatus status {};
        std::optional<std::string> data {};
        CHARGELAB_JSON_INTRUSIVE(DataTransferRsp, status, data)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_DATA_TRANSFER_H
