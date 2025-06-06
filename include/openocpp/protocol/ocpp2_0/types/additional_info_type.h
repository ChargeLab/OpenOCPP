#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_ADDITIONAL_INFO_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_ADDITIONAL_INFO_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct AdditionalInfoType {
        IdentifierStringPrimitive<36> additionalIdToken {};
        StringPrimitive<50> type {};
        CHARGELAB_JSON_INTRUSIVE(AdditionalInfoType, additionalIdToken, type)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_ADDITIONAL_INFO_TYPE_H
