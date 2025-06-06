#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SIGN_CERTIFICATE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SIGN_CERTIFICATE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/certificate_signing_use_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/generic_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SignCertificateRequest {
        StringPrimitive<5500> csr {};
        std::optional<CertificateSigningUseEnumType> certificateType {};
        CHARGELAB_JSON_INTRUSIVE_CALL(SignCertificateRequest, kSignCertificate, csr, certificateType)
    };


    struct SignCertificateResponse {
        GenericStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(SignCertificateResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SIGN_CERTIFICATE_H
