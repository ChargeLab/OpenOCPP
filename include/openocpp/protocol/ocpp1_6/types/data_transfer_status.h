#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_DATA_TRANSFER_STATUS_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_DATA_TRANSFER_STATUS_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(DataTransferStatus, 
        Accepted,
        Rejected,
        UnknownMessageId,
        UnknownVendorId
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_DATA_TRANSFER_STATUS_H
