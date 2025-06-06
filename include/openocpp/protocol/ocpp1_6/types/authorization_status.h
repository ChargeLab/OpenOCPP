#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_AUTHORIZATION_STATUS_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_AUTHORIZATION_STATUS_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(AuthorizationStatus,
        Accepted,
        Blocked,
        Expired,
        Invalid,
        ConcurrentTx
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_AUTHORIZATION_STATUS_H
