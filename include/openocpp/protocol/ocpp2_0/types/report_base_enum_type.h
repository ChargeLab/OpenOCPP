#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_REPORT_BASE_ENUM_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_REPORT_BASE_ENUM_TYPE_H

#include "openocpp/helpers/json.h"

namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(ReportBaseEnumType, 
        ConfigurationInventory,
        FullInventory,
        SummaryInventory
    )
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_REPORT_BASE_ENUM_TYPE_H
