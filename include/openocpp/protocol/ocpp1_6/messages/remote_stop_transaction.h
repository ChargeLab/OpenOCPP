#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_REMOTE_STOP_TRANSACTION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_REMOTE_STOP_TRANSACTION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/protocol/ocpp1_6/types/remote_start_stop_status.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct RemoteStopTransactionReq {
        int transactionId;
        CHARGELAB_JSON_INTRUSIVE_CALL(RemoteStopTransactionReq, kRemoteStopTransaction, transactionId)
    };

    struct RemoteStopTransactionRsp {
        RemoteStartStopStatus status;
        CHARGELAB_JSON_INTRUSIVE(RemoteStopTransactionRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_REMOTE_STOP_TRANSACTION_H
