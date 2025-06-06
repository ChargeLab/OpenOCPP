#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_FIRMWARE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_FIRMWARE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct FirmwareType {
        StringPrimitive<512> location {};
        DateTime retrieveDateTime {};
        std::optional<DateTime> installDateTime {};
        std::optional<StringPrimitive<5500>> signingCertificate {};
        std::optional<StringPrimitive<800>> signature {};
        CHARGELAB_JSON_INTRUSIVE(FirmwareType, location, retrieveDateTime, installDateTime, signingCertificate, signature)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_FIRMWARE_TYPE_H
