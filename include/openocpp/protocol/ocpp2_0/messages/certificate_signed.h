#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_SIGNED_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_SIGNED_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/certificate_signing_use_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/certificate_signed_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct CertificateSignedRequest {
        StringPrimitive<10000> certificateChain {};
        std::optional<CertificateSigningUseEnumType> certificateType {};
        CHARGELAB_JSON_INTRUSIVE_CALL(CertificateSignedRequest, kCertificateSigned, certificateChain, certificateType)
    };


    struct CertificateSignedResponse {
        CertificateSignedStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(CertificateSignedResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_SIGNED_H
