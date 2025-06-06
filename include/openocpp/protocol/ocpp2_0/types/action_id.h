#ifndef CHARGELAB_OPEN_FIRMWARE_2_0_ACTION_ID_H
#define CHARGELAB_OPEN_FIRMWARE_2_0_ACTION_ID_H

#include "openocpp/helpers/json.h"

#define CHARGELAB_OCPP_2_0_ACTION_IDS             \
    Authorize,                                    \
    BootNotification,                             \
    CancelReservation,                            \
    CertificateSigned,                            \
    ChangeAvailability,                           \
    ClearCache,                                   \
    ClearChargingProfile,                         \
    ClearDisplayMessage,                          \
    ClearVariableMonitoring,                      \
    ClearedChargingLimit,                         \
    CostUpdated,                                  \
    CustomerInformation,                          \
    DataTransfer,                                 \
    DeleteCertificate,                            \
    FirmwareStatusNotification,                   \
    Get15118EVCertificate,                        \
    GetBaseReport,                                \
    GetCertificateStatus,                         \
    GetChargingProfiles,                          \
    GetCompositeSchedule,                         \
    GetDisplayMessages,                           \
    GetInstalledCertificateIds,                   \
    GetLocalListVersion,                          \
    GetLog,                                       \
    GetMonitoringReport,                          \
    GetReport,                                    \
    GetTransactionStatus,                         \
    GetVariables,                                 \
    Heartbeat,                                    \
    InstallCertificate,                           \
    LogStatusNotification,                        \
    MeterValues,                                  \
    NotifyChargingLimit,                          \
    NotifyCustomerInformation,                    \
    NotifyDisplayMessages,                        \
    NotifyEVChargingNeeds,                        \
    NotifyEVChargingSchedule,                     \
    NotifyEvent,                                  \
    NotifyMonitoringReport,                       \
    NotifyReport,                                 \
    PublishFirmware,                              \
    PublishFirmwareStatusNotification,            \
    ReportChargingProfiles,                       \
    RequestStartTransaction,                      \
    RequestStopTransaction,                       \
    ReservationStatusUpdate,                      \
    ReserveNow,                                   \
    Reset,                                        \
    SecurityEventNotification,                    \
    SendLocalList,                                \
    SetChargingProfile,                           \
    SetDisplayMessage,                            \
    SetMonitoringBase,                            \
    SetMonitoringLevel,                           \
    SetNetworkProfile,                            \
    SetVariableMonitoring,                        \
    SetVariables,                                 \
    SignCertificate,                              \
    StatusNotification,                           \
    TransactionEvent,                             \
    TriggerMessage,                               \
    UnlockConnector,                              \
    UnpublishFirmware,                            \
    UpdateFirmware


namespace chargelab::ocpp2_0 {
    CHARGELAB_JSON_ENUM(ActionId, CHARGELAB_OCPP_2_0_ACTION_IDS)
}

#endif //CHARGELAB_OPEN_FIRMWARE_2_0_ACTION_ID_H
