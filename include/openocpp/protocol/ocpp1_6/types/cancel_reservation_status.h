#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_CANCEL_RESERVATION_STATUS_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_CANCEL_RESERVATION_STATUS_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(CancelReservationStatus,
        Accepted,
        Rejected
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_CANCEL_RESERVATION_STATUS_H
