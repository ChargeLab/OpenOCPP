#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_INSTALL_CERTIFICATE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_INSTALL_CERTIFICATE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/install_certificate_use_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/install_certificate_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct InstallCertificateRequest {
        InstallCertificateUseEnumType certificateType {};
        StringPrimitive<5500> certificate {};
        CHARGELAB_JSON_INTRUSIVE_CALL(InstallCertificateRequest, kInstallCertificate, certificateType, certificate)
    };


    struct InstallCertificateResponse {
        InstallCertificateStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(InstallCertificateResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_INSTALL_CERTIFICATE_H
