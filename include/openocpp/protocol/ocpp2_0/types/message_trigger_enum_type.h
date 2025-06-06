#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_TRIGGER_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_TRIGGER_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(MessageTriggerEnumType, 
        BootNotification,
        LogStatusNotification,
        FirmwareStatusNotification,
        Heartbeat,
        MeterValues,
        SignChargingStationCertificate,
        SignV2GCertificate,
        StatusNotification,
        TransactionEvent,
        SignCombinedCertificate,
        PublishFirmwareStatusNotification
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_MESSAGE_TRIGGER_ENUM_TYPE_H
