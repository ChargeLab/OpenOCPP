#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_START_TRANSACTION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_START_TRANSACTION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/ci_string_type.h"
#include "openocpp/protocol/ocpp1_6/types/id_token.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/id_tag_info.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct StartTransactionReq {
        int connectorId;
        IdToken idTag;
        int meterStart;
        std::optional<int> reservationId;
        DateTime timestamp;
        CHARGELAB_JSON_INTRUSIVE_CALL(StartTransactionReq, kStartTransaction, connectorId, idTag, meterStart, reservationId, timestamp)
    };

    struct StartTransactionRsp {
        IdTagInfo idTagInfo;
        int transactionId;
        CHARGELAB_JSON_INTRUSIVE(StartTransactionRsp, idTagInfo, transactionId)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_START_TRANSACTION_H
