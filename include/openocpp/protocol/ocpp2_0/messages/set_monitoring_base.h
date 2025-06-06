#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_BASE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_BASE_H

#include "openocpp/protocol/ocpp2_0/types/action_id.h"
#include "openocpp/protocol/ocpp2_0/types/monitoring_base_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/generic_device_model_status_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/status_info_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct SetMonitoringBaseRequest {
        MonitoringBaseEnumType monitoringBase {};
        CHARGELAB_JSON_INTRUSIVE_CALL(SetMonitoringBaseRequest, kSetMonitoringBase, monitoringBase)
    };


    struct SetMonitoringBaseResponse {
        GenericDeviceModelStatusEnumType status {};
        std::optional<StatusInfoType> statusInfo {};
        CHARGELAB_JSON_INTRUSIVE(SetMonitoringBaseResponse, status, statusInfo)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_SET_MONITORING_BASE_H
