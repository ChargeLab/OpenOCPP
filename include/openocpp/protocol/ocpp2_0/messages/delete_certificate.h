#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_DELETE_CERTIFICATE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_DELETE_CERTIFICATE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/certificate_hash_data_type.h"
#include "openocpp/protocol/ocpp2_0/types/delete_certificate_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct DeleteCertificateRequest {
        CertificateHashDataType certificateHashData {};
        CHARGELAB_JSON_INTRUSIVE_CALL(DeleteCertificateRequest, kDeleteCertificate, certificateHashData)
    };


    struct DeleteCertificateResponse {
        DeleteCertificateStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(DeleteCertificateResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_DELETE_CERTIFICATE_H
