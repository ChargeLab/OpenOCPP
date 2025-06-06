#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_HASH_DATA_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_HASH_DATA_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/hash_algorithm_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct CertificateHashDataType {
        HashAlgorithmEnumType hashAlgorithm {};
        IdentifierStringPrimitive<128> issuerNameHash {};

        // Note: deviating from the specification so that a *case-insensitive* comparison is used here
        IdentifierStringPrimitive<128> issuerKeyHash {};

        IdentifierStringPrimitive<40> serialNumber {};
        CHARGELAB_JSON_INTRUSIVE(CertificateHashDataType, hashAlgorithm, issuerNameHash, issuerKeyHash, serialNumber)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_HASH_DATA_TYPE_H
