#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_STOP_TRANSACTION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_STOP_TRANSACTION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/id_token.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/reason.h"
#include "openocpp/protocol/ocpp1_6/types/meter_value.h"
#include "openocpp/protocol/ocpp1_6/types/id_tag_info.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct StopTransactionReq {
        IdToken idTag;
        int meterStop;
        DateTime timestamp;
        int transactionId;
        Reason reason;
        std::vector<MeterValue> transactionData;
        CHARGELAB_JSON_INTRUSIVE_CALL(StopTransactionReq, kStopTransaction, idTag, meterStop, timestamp, transactionId, reason, transactionData)
    };

    struct StopTransactionRsp {
        std::optional<IdTagInfo> idTagInfo;
        CHARGELAB_JSON_INTRUSIVE(StopTransactionRsp, idTagInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_STOP_TRANSACTION_H
