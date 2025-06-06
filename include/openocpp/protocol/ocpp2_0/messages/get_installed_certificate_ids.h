#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_INSTALLED_CERTIFICATE_IDS_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_INSTALLED_CERTIFICATE_IDS_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/get_certificate_id_use_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/get_installed_certificate_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/certificate_hash_data_chain_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/protocol/common/vector.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct GetInstalledCertificateIdsRequest {
        std::optional<std::vector<GetCertificateIdUseEnumType>> certificateType {};
        CHARGELAB_JSON_INTRUSIVE_CALL(GetInstalledCertificateIdsRequest, kGetInstalledCertificateIds, certificateType)
    };


    struct GetInstalledCertificateIdsResponse {
        GetInstalledCertificateStatusEnumType status {};
        Vector<CertificateHashDataChainType, true> certificateHashDataChain {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(GetInstalledCertificateIdsResponse, status, certificateHashDataChain, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_INSTALLED_CERTIFICATE_IDS_H
