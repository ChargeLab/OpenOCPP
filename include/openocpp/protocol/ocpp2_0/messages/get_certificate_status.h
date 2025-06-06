#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_CERTIFICATE_STATUS_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_CERTIFICATE_STATUS_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/ocsp_request_data_type.h"
#include "openocpp/protocol/ocpp2_0/types/get_certificate_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetCertificateStatusRequest {
        OCSPRequestDataType ocspRequestData {};
        CHARGELAB_JSON_INTRUSIVE_CALL(GetCertificateStatusRequest, kGetCertificateStatus, ocspRequestData)
    };


    struct GetCertificateStatusResponse {
        GetCertificateStatusEnumType status {};
        std::optional<StringPrimitive<5500>> ocspResult {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(GetCertificateStatusResponse, status, ocspResult, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_CERTIFICATE_STATUS_H
