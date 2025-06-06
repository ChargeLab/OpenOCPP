#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SIGNED_METER_VALUE_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SIGNED_METER_VALUE_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SignedMeterValueType {
        StringPrimitive<2500> signedMeterData {};
        StringPrimitive<50> signingMethod {};
        StringPrimitive<50> encodingMethod {};
        StringPrimitive<2500> publicKey {};
        CHARGELAB_JSON_INTRUSIVE(SignedMeterValueType, signedMeterData, signingMethod, encodingMethod, publicKey)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SIGNED_METER_VALUE_TYPE_H
