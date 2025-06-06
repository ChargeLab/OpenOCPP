#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_STATUS_INFO_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_STATUS_INFO_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct StatusInfoType {
        StringPrimitive<20> reasonCode {};
        std::optional<StringPrimitive<512>> additionalInfo {};
        CHARGELAB_JSON_INTRUSIVE(StatusInfoType, reasonCode, additionalInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_STATUS_INFO_TYPE_H
