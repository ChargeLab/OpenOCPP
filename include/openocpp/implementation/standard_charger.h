#ifndef CHARGELAB_OPEN_FIRMWARE_STANDARD_CHARGER_H
#define CHARGELAB_OPEN_FIRMWARE_STANDARD_CHARGER_H

#include "openocpp/interface/platform_interface.h"
#include "openocpp/interface/station_interface.h"
#include "openocpp/common/settings.h"
#include "openocpp/module/pending_messages_module.h"
#include "openocpp/module/connector_status_module.h"
#include "openocpp/module/get_logs_module.h"
#include "openocpp/module/boot_notification_module.h"
#include "openocpp/module/heartbeat_module.h"
#include "openocpp/module/configuration_module.h"
#include "openocpp/module/fallback_module.h"
#include "openocpp/module/reset_module.h"
#include "openocpp/module/firmware_update_module.h"
#include "openocpp/module/power_management_module1_6.h"
#include "openocpp/module/transaction_module1_6.h"
#include "openocpp/protocol/ocpp1_6/handlers/ocpp_message_handler.h"
#include "openocpp/module/power_management_module2_0.h"
#include "openocpp/module/transaction_module2_0.h"
#include "openocpp/protocol/ocpp2_0/handlers/ocpp_message_handler.h"

#include "openocpp/implementation/hash_methods_mbedtls.h"

namespace chargelab {
    class StandardCharger {
    public:
        std::shared_ptr<PlatformInterface> platform;
        std::shared_ptr<StationInterface> station;
        std::shared_ptr<Settings> settings;

        std::shared_ptr<PendingMessagesModule> pending_messages_module;
        std::shared_ptr<ConnectorStatusModule> connector_status_module;
        std::shared_ptr<GetLogsModule> get_logs_module;
        std::shared_ptr<BootNotificationModule> boot_notification_module;
        std::shared_ptr<HeartbeatModule> heartbeat_module;
        std::shared_ptr<ConfigurationModule> configuration_module;
        std::shared_ptr<FallbackModule> fallback_module;
        std::shared_ptr<ResetModule> reset_module;
        std::shared_ptr<FirmwareUpdateModule<HashMethodsMbedTLS>> firmware_update_module;

        std::shared_ptr<PowerManagementModule1_6> power_management_module1_6;
        std::shared_ptr<TransactionModule1_6> transaction_module1_6;
        std::shared_ptr<ocpp1_6::OcppMessageHandler> message_handler1_6;

        std::shared_ptr<PowerManagementModule2_0> power_management_module2_0;
        std::shared_ptr<TransactionModule2_0> transaction_module2_0;
        std::shared_ptr<ocpp2_0::OcppMessageHandler> message_handler2_0;

    public:
        template <typename T>
        T notNull(T value) {
            assert(value != nullptr);
            return value;
        }

        StandardCharger(
                std::shared_ptr<PlatformInterface> platform_interface,
                std::shared_ptr<StationInterface> station_interface
        )
                : platform(std::move(platform_interface)),
                  station(std::move(station_interface))
        {
            settings = platform->getSettings();

            CHARGELAB_LOG_MESSAGE(info) << "Initializing common modules...";
            pending_messages_module = std::make_shared<PendingMessagesModule>(
                    notNull(settings),
                    notNull(platform),
                    platform->getStorage("transactions.dat")
            );
            connector_status_module = std::make_shared<ConnectorStatusModule>(
                    notNull(settings),
                    notNull(platform),
                    notNull(station)
            );

            get_logs_module = std::make_shared<GetLogsModule>(notNull(platform), notNull(pending_messages_module));
            boot_notification_module = std::make_shared<BootNotificationModule>(notNull(settings), notNull(platform));
            heartbeat_module = std::make_shared<HeartbeatModule>(notNull(settings), notNull(platform));
            configuration_module = std::make_shared<ConfigurationModule>(notNull(settings), notNull(platform));
            fallback_module = std::make_shared<FallbackModule>(notNull(platform));
            reset_module = std::make_shared<ResetModule>(notNull(settings), notNull(platform), notNull(connector_status_module));

            // TODO: Maybe hash methods should move into platform?
            firmware_update_module = std::make_shared<FirmwareUpdateModule<HashMethodsMbedTLS>>(
                    notNull(platform),
                            notNull(reset_module),
                            notNull(pending_messages_module),
                            notNull(connector_status_module),
                            notNull(station)
            );

            CHARGELAB_LOG_MESSAGE(info) << "Initializing OCPP 1.6 modules...";
            power_management_module1_6 = std::make_shared<PowerManagementModule1_6>(
                    notNull(settings),
                    notNull(platform),
                    notNull(station),
                    platform->getPartition("pmjournal")
            );

            transaction_module1_6 = std::make_shared<TransactionModule1_6>(
                    notNull(platform),
                    notNull(boot_notification_module),
                    notNull(power_management_module1_6),
                    notNull(pending_messages_module),
                    notNull(connector_status_module),
                    notNull(station)
            );

            message_handler1_6 = std::make_shared<ocpp1_6::OcppMessageHandler>(
                    notNull(settings),
                    notNull(platform),
                    std::vector<std::shared_ptr<AbstractModuleInterface>> {
                            notNull(platform),
                            notNull(boot_notification_module),
                            notNull(heartbeat_module),
                            notNull(configuration_module),
                            notNull(reset_module),
                            notNull(connector_status_module),
                            notNull(pending_messages_module),
                            notNull(firmware_update_module),
                            notNull(get_logs_module),
                            notNull(power_management_module1_6),
                            notNull(transaction_module1_6),
                            notNull(fallback_module)
                    },
                    // TODO: Get rid of this lambda; more expensive than a pointer? Also less clear what's being invoked at the call-site.
                    [this]() { return boot_notification_module->registrationComplete(); }
            );

            CHARGELAB_LOG_MESSAGE(info) << "Initializing OCPP 2.0 modules...";
            power_management_module2_0 = std::make_shared<PowerManagementModule2_0>(
                    notNull(settings),
                    notNull(platform),
                    notNull(station),
                    platform->getPartition("pmjournal")
            );

            transaction_module2_0 = std::make_shared<TransactionModule2_0>(
                    notNull(platform),
                    notNull(boot_notification_module),
                    notNull(power_management_module2_0),
                    notNull(pending_messages_module),
                    notNull(connector_status_module),
                    notNull(station)
            );

            message_handler2_0 = std::make_shared<ocpp2_0::OcppMessageHandler>(
                    notNull(settings),
                    notNull(platform),
                    std::vector<std::shared_ptr<AbstractModuleInterface>> {
                            platform,
                            boot_notification_module,
                            heartbeat_module,
                            configuration_module,
                            reset_module,
                            connector_status_module,
                            pending_messages_module,
                            firmware_update_module,
                            get_logs_module,
                            power_management_module2_0,
                            transaction_module2_0,
                            fallback_module
                    },
                    // TODO: As above; remove this?
                    [this]() { return boot_notification_module->registrationComplete(); }
            );
        }

        void runStep() {
            message_handler1_6->runStep(platform->ocppConnection());
            message_handler2_0->runStep(platform->ocppConnection());
        }

        void runLoop() {
            while (true) {
                message_handler1_6->runStep(platform->ocppConnection());
                message_handler2_0->runStep(platform->ocppConnection());

                using namespace std::chrono_literals;
                std::this_thread::sleep_for(10ms);
            }
        }

        template <typename T, typename... Args>
        void addAfter(std::shared_ptr<T> module, Args&&... args) {
            message_handler1_6->addAfter(module, std::forward<Args>(args)...);
            message_handler2_0->addAfter(module, std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        void addBefore(std::shared_ptr<T> module, Args&&... args) {
            message_handler1_6->addBefore(module, std::forward<Args>(args)...);
            message_handler2_0->addBefore(module, std::forward<Args>(args)...);
        }
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_STANDARD_CHARGER_H
