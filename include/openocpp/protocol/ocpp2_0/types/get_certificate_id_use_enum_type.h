#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_GET_CERTIFICATE_ID_USE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_GET_CERTIFICATE_ID_USE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(GetCertificateIdUseEnumType, 
        V2GRootCertificate,
        MORootCertificate,
        CSMSRootCertificate,
        V2GCertificateChain,
        ManufacturerRootCertificate
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_GET_CERTIFICATE_ID_USE_ENUM_TYPE_H
