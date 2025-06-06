#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_OCSP_REQUEST_DATA_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_OCSP_REQUEST_DATA_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/hash_algorithm_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct OCSPRequestDataType {
        HashAlgorithmEnumType hashAlgorithm {};
        IdentifierStringPrimitive<128> issuerNameHash {};
        StringPrimitive<128> issuerKeyHash {};
        IdentifierStringPrimitive<40> serialNumber {};
        StringPrimitive<512> responderURL {};
        CHARGELAB_JSON_INTRUSIVE(OCSPRequestDataType, hashAlgorithm, issuerNameHash, issuerKeyHash, serialNumber, responderURL)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_OCSP_REQUEST_DATA_TYPE_H
