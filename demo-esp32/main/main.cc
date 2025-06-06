#include "portal_demo.h"
#include "openocpp/implementation/platform_esp.h"
#include "openocpp/implementation/standard_charger.h"
#include "openocpp/implementation/station_test_esp32.h"
#include "openocpp/common/logging.h"

#include "driver/gpio.h"

extern "C" void app_main(void) {
    chargelab::logging::SetLogLevel(chargelab::logging::LogLevel::debug);

    auto firstBootAfterFactoryReset = std::make_shared<chargelab::SettingBool> (
        []() {
            return chargelab::SettingMetadata {
                "FirstBootAfterFactoryReset",
                chargelab::SettingConfig::rwPolicy(),
                std::nullopt,
                std::nullopt,
                chargelab::SettingBool::kTextTrue
            };
        },
        [](auto const&) {return true;}
    );

    auto platform = std::make_shared<chargelab::PlatformESP>();
    auto settings = platform->getSettings();
    settings->registerCustomSetting(firstBootAfterFactoryReset);

    if (firstBootAfterFactoryReset->getValue()) {
        // Clear certificates and install default certificates
        platform->removeCertificatesIf([](auto const&) {return true;});

        extern const unsigned char _binary_manufacturer_pem_start[]        asm("_binary_manufacturer_pem_start");
        extern const unsigned char _binary_manufacturer_pem_end[]          asm("_binary_manufacturer_pem_end");
        std::string pem {_binary_manufacturer_pem_start, _binary_manufacturer_pem_end};
        if (!platform->addCertificate(pem, chargelab::ocpp2_0::GetCertificateIdUseEnumType::kManufacturerRootCertificate)) {
            CHARGELAB_LOG_MESSAGE(warning) << "Failed adding default manufacturer root CA: " << pem;
        } else {
            CHARGELAB_LOG_MESSAGE(info) << "Added default manufacturer root CA: " << pem;
        }

        firstBootAfterFactoryReset->setValue(false);
    }

    CHARGELAB_LOG_MESSAGE(info) << "Current setting";
    settings->visitSettings([](auto const& x) {
        CHARGELAB_LOG_MESSAGE(info) << x.getId() << ": " << x.getValueAsString();
    });

    auto station_test = std::make_shared<chargelab::StationTestEsp32>(platform);
    auto charger = std::make_unique<chargelab::StandardCharger>(platform, station_test);
    charger->addAfter(std::make_shared<chargelab::PortalDemo>(platform, charger->reset_module,
                                                              charger->connector_status_module,
                                                              charger->configuration_module,
                                                              station_test));

    std::optional<chargelab::SteadyPointMillis> held_since;
    while (true) {
        station_test->updateMeasurements();
        charger->runStep();

        auto const now = platform->steadyClockNow();
        if (gpio_get_level(GPIO_NUM_0) == 0) {
            if (!held_since.has_value())
                held_since = platform->steadyClockNow();

            if (now - held_since.value() > 5000) {
                CHARGELAB_LOG_MESSAGE(info) << "Factory reset requested - clearing all saved state";
                for (auto const& name : std::vector<std::string> {"pmjournal", "spiffs"}) {
                    auto partition = platform->getPartition(name);
                    if (partition != nullptr) {
                        if (!partition->erase()) {
                            CHARGELAB_LOG_MESSAGE(error) << "Failed erasing partition: " << name;
                        }
                    }
                }

                platform->resetHard();
            }
        } else {
            held_since = std::nullopt;
        }

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }
}
