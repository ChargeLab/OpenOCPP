#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_HASH_ALGORITHM_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_HASH_ALGORITHM_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(HashAlgorithmEnumType, 
        SHA256,
        SHA384,
        SHA512
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_HASH_ALGORITHM_ENUM_TYPE_H
