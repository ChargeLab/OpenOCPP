#ifndef CHARGELAB_OPEN_FIRMWARE_GET_15118_EV_CERTIFICATE_H
#define CHARGELAB_OPEN_FIRMWARE_GET_15118_EV_CERTIFICATE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/certificate_action_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/iso_15118_ev_certificate_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct Get15118EVCertificateRequest {
        StringPrimitive<50> iso15118SchemaVersion {};
        CertificateActionEnumType action {};
        StringPrimitive<5600> exiRequest {};
        CHARGELAB_JSON_INTRUSIVE_CALL(Get15118EVCertificateRequest, kGet15118EVCertificate, iso15118SchemaVersion, action, exiRequest)
    };


    struct Get15118EVCertificateResponse {
        Iso15118EVCertificateStatusEnumType status {};
        StringPrimitive<5600> exiResponse {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(Get15118EVCertificateResponse, status, exiResponse, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_GET_15118_EV_CERTIFICATE_H
