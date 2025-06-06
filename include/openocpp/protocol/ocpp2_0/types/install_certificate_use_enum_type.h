#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_INSTALL_CERTIFICATE_USE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_INSTALL_CERTIFICATE_USE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(InstallCertificateUseEnumType, 
        V2GRootCertificate,
        MORootCertificate,
        CSMSRootCertificate,
        ManufacturerRootCertificate
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_INSTALL_CERTIFICATE_USE_ENUM_TYPE_H
