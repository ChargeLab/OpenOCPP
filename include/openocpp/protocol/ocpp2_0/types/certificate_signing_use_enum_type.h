#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_SIGNING_USE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_SIGNING_USE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(CertificateSigningUseEnumType, 
        ChargingStationCertificate,
        V2GCertificate
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_CERTIFICATE_SIGNING_USE_ENUM_TYPE_H
