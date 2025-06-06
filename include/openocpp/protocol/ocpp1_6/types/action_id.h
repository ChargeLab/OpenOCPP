#ifndef CHARGELAB_OPEN_FIRMWARE_1_6_ACTION_ID_H
#define CHARGELAB_OPEN_FIRMWARE_1_6_ACTION_ID_H

#include "openocpp/helpers/json.h"

#define CHARGELAB_OCPP_1_6_ACTION_IDS             \
    Authorize,                                    \
    BootNotification,                             \
    CancelReservation,                            \
    ChangeAvailability,                           \
    ChangeConfiguration,                          \
    ClearCache,                                   \
    ClearChargingProfile,                         \
    DataTransfer,                                 \
    DiagnosticsStatusNotification,                \
    FirmwareStatusNotification,                   \
    GetCompositeSchedule,                         \
    GetConfiguration,                             \
    GetDiagnostics,                               \
    GetLocalListVersion,                          \
    Heartbeat,                                    \
    MeterValues,                                  \
    RemoteStartTransaction,                       \
    RemoteStopTransaction,                        \
    ReserveNow,                                   \
    Reset,                                        \
    SendLocalList,                                \
    SetChargingProfile,                           \
    StartTransaction,                             \
    StatusNotification,                           \
    StopTransaction,                              \
    TriggerMessage,                               \
    UnlockConnector,                              \
    UpdateFirmware

namespace chargelab::ocpp1_6 {
    CHARGELAB_JSON_ENUM(ActionId, CHARGELAB_OCPP_1_6_ACTION_IDS)
}

#endif //CHARGELAB_OPEN_FIRMWARE_1_6_ACTION_ID_H
