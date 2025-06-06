#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_RESERVATION_STATUS_UPDATE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_RESERVATION_STATUS_UPDATE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/reservation_update_status_enum_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct ReservationStatusUpdateRequest {
        int reservationId {};
        ReservationUpdateStatusEnumType reservationUpdateStatus {};
        CHARGELAB_JSON_INTRUSIVE_CALL(ReservationStatusUpdateRequest, kReservationStatusUpdate, reservationId, reservationUpdateStatus)
    };


    struct ReservationStatusUpdateResponse {
        CHARGELAB_JSON_INTRUSIVE_EMPTY(ReservationStatusUpdateResponse)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_RESERVATION_STATUS_UPDATE_H
