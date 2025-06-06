#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_RESERVE_NOW_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_RESERVE_NOW_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/date_time.h"
#include "openocpp/protocol/ocpp1_6/types/id_token.h"
#include "openocpp/protocol/ocpp1_6/types/reservation_status.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp1_6 {
    struct ReserveNowReq {
        int connectorId;
        DateTime expiryDate;
        IdToken idTag;
        std::optional<IdToken> parentIdTag;
        int reservationId;
        CHARGELAB_JSON_INTRUSIVE_CALL(ReserveNowReq, kReserveNow, connectorId, expiryDate, idTag, parentIdTag, reservationId)
    };

    struct ReserveNowRsp {
        ReservationStatus status;
        CHARGELAB_JSON_INTRUSIVE(ReserveNowRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_RESERVE_NOW_H
