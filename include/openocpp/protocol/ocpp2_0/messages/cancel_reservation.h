#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CANCEL_RESERVATION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CANCEL_RESERVATION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/cancel_reservation_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct CancelReservationRequest {
        int reservationId {};
        CHARGELAB_JSON_INTRUSIVE_CALL(CancelReservationRequest, kCancelReservation, reservationId)
    };


    struct CancelReservationResponse {
        CancelReservationStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(CancelReservationResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CANCEL_RESERVATION_H
