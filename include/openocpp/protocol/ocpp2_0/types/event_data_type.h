#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_EVENT_DATA_TYPE_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_EVENT_DATA_TYPE_H

#include "openocpp/protocol/ocpp2_0/types/date_time.h"
#include "openocpp/protocol/ocpp2_0/types/event_trigger_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/identifier_string_primitive.h"
#include "openocpp/protocol/ocpp2_0/types/event_notification_enum_type.h"
#include "openocpp/protocol/ocpp2_0/types/component_type.h"
#include "openocpp/protocol/ocpp2_0/types/variable_type.h"
#include "openocpp/helpers/json.h"

#include <optional>

namespace chargelab::ocpp2_0 {
    struct EventDataType {
        int eventId {};
        DateTime timestamp {};
        EventTriggerEnumType trigger {};
        std::optional<int> cause {};
        StringPrimitive<2500> actualValue {};
        std::optional<StringPrimitive<50>> techCode {};
        std::optional<StringPrimitive<500>> techInfo {};
        std::optional<bool> cleared {};
        std::optional<IdentifierStringPrimitive<36>> transactionId {};
        std::optional<int> variableMonitoringId {};
        EventNotificationEnumType eventNotificationType {};
        ComponentType component {};
        VariableType variable {};
        CHARGELAB_JSON_INTRUSIVE(EventDataType, eventId, timestamp, trigger, cause, actualValue, techCode, techInfo, cleared, transactionId, variableMonitoringId, eventNotificationType, component, variable)
    };

}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_EVENT_DATA_TYPE_H
