#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_AUTHORIZE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_AUTHORIZE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/id_token_type.h"
#include "openocpp/protocol/ocpp2_0/types/ocsp_request_data_type.h"
#include "openocpp/protocol/ocpp2_0/types/authorize_certificate_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/id_token_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct AuthorizeRequest {
        std::optional<StringPrimitive<5500>> certificate {};
        IdTokenType idToken {};
        std::optional<std::vector<OCSPRequestDataType>> iso15118CertificateHashData {};
        CHARGELAB_JSON_INTRUSIVE_CALL(AuthorizeRequest, kAuthorize, certificate, idToken, iso15118CertificateHashData)
    };


    struct AuthorizeResponse {
        std::optional<AuthorizeCertificateStatusEnumType> certificateStatus {};
        IdTokenInfoType idTokenInfo {};
        CHARGELAB_JSON_INTRUSIVE(AuthorizeResponse, certificateStatus, idTokenInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_AUTHORIZE_H
