#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CANCEL_RESERVATION_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CANCEL_RESERVATION_H

#include "openocpp/protocol/ocpp1_6/types/action_id.h"
#include "openocpp/protocol/ocpp1_6/types/cancel_reservation_status.h"
#include "openocpp/helpers/json.h"

#include <string>

namespace chargelab::ocpp1_6 {
    struct CancelReservationReq {
        int reservationId;
        CHARGELAB_JSON_INTRUSIVE_CALL(CancelReservationReq, kCancelReservation, reservationId)
    };

    struct CancelReservationRsp {
        CancelReservationStatus status;
        CHARGELAB_JSON_INTRUSIVE(CancelReservationRsp, status)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CANCEL_RESERVATION_H
