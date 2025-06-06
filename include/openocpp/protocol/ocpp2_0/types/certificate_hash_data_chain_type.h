#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_HASH_DATA_CHAIN_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_HASH_DATA_CHAIN_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/get_certificate_id_use_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/certificate_hash_data_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct CertificateHashDataChainType {
        GetCertificateIdUseEnumType certificateType {};
        CertificateHashDataType certificateHashData {};
        std::optional<std::vector<CertificateHashDataType>> childCertificateHashData {};
        CHARGELAB_JSON_INTRUSIVE(CertificateHashDataChainType, certificateType, certificateHashData, childCertificateHashData)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_HASH_DATA_CHAIN_TYPE_H
