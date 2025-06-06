#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_RECURRENCY_KIND_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_RECURRENCY_KIND_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(RecurrencyKindType, 
        Daily,
        Weekly
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_RECURRENCY_KIND_TYPE_H
