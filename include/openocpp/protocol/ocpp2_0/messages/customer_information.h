#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CUSTOMER_INFORMATION_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CUSTOMER_INFORMATION_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/id_token_type.h"
#include "openocpp/protocol/ocpp2_0/types/certificate_hash_data_type.h"
#include "openocpp/protocol/ocpp2_0/types/customer_information_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct CustomerInformationRequest {
        int requestId {};
        bool report {};
        bool clear {};
        std::optional<StringPrimitive<64>> customerIdentifier {};
        std::optional<IdTokenType> idToken {};
        std::optional<CertificateHashDataType> customerCertificate {};
        CHARGELAB_JSON_INTRUSIVE_CALL(CustomerInformationRequest, kCustomerInformation, requestId, report, clear, customerIdentifier, idToken, customerCertificate)
    };


    struct CustomerInformationResponse {
        CustomerInformationStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(CustomerInformationResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CUSTOMER_INFORMATION_H
