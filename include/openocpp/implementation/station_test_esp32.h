#ifndef CHARGELAB_OPEN_FIRMWARE_STATION_TEST_ESP32_H
#define CHARGELAB_OPEN_FIRMWARE_STATION_TEST_ESP32_H

#include "openocpp/implementation/station_esp32.h"
#include "openocpp/interface/platform_interface.h"
#include "openocpp/helpers/string.h"
#include "openocpp/version.h"

#include <random>

namespace chargelab {
    namespace detail {
        struct SimulatorConnectorState {
            bool connector_available {true};
            bool vehicle_connected {false};
            bool suspended_by_vehicle {false};
            bool suspended_by_charger {false};

            std::optional<SteadyPointMillis> last_update = std::nullopt;
            double amps {48.0};
            double volts {220.0};
            double watt_hours {0.0};

            // Note: not saved
            std::optional<double> limit = std::nullopt;
            bool charging_enabled {false};

            CHARGELAB_JSON_INTRUSIVE(
                    SimulatorConnectorState,
                    connector_available,
                    vehicle_connected,
                    suspended_by_vehicle,
                    suspended_by_charger,
                    last_update,
                    amps,
                    volts,
                    watt_hours
            )
        };

        struct SimulatorState {
            charger::StationMetadata station_metadata {};
            std::map<ocpp2_0::EVSEType, charger::EvseMetadata> evse_metadata {};
            std::map<ocpp2_0::EVSEType, charger::ConnectorMetadata> connector_metadata {};
            std::map<ocpp2_0::EVSEType, SimulatorConnectorState> connector_state {};

            // Note: connector state is not saved intentionally
            CHARGELAB_JSON_INTRUSIVE(SimulatorState, station_metadata, evse_metadata, connector_metadata, connector_state)
        };
    }

    class StationTestEsp32 : public StationEsp32 {
    public:
        StationTestEsp32(std::shared_ptr<PlatformInterface> platform)
                : platform_(std::move(platform)),
                  random_engine_ {std::random_device{}()}
        {
            assert(platform_ != nullptr);
            settings_ = platform_->getSettings();
            assert(settings_ != nullptr);

            // TODO: Move these into SimulatorState so that they're persisted and can be configured?
            settings_->ChargerVendor.setValue("ChargeLab");
            settings_->ChargerModel.setValue("Simulator");
            settings_->ChargerSerialNumber.setValue("0000");
            settings_->ChargerFirmwareVersion.setValue(OPENOCPP_VERSION_TEXT);
            settings_->ChargerAccessPointSSID.setValue("Charger Simulator");

            settings_->registerCustomSetting(saved_state_ = std::make_shared<SettingString>(
                    []() {
                        detail::SimulatorState default_state;
                        default_state.station_metadata = charger::StationMetadata {
                                1
                        };
                        default_state.evse_metadata[ocpp2_0::EVSEType {1}] = charger::EvseMetadata {
                                1,
                                10560.0
                        };
                        default_state.connector_metadata[ocpp2_0::EVSEType {1, 1}] = charger::ConnectorMetadata {
                                1,
                                "cType1",
                                1,
                                10560.0,
                                48
                        };

                        return SettingMetadata {
                                "SimulatorState",
                                SettingConfig::rwPolicy(),
                                std::nullopt,
                                std::nullopt,
                                write_json_to_string(default_state)
                        };
                    },
                    [](auto const&) {return true;}
            ));
        }

    public:
        void simulateTap(ocpp1_6::IdToken token1_6, ocpp2_0::IdTokenType token2_0, int timeout_millis) {
            std::lock_guard lock {mutex_};
            rfid_action_ = std::make_pair(
                    static_cast<SteadyPointMillis>(platform_->steadyClockNow() + timeout_millis),
                    std::make_pair(token1_6, token2_0)
            );
        }

        void updateMeasurements() {
            std::lock_guard lock {mutex_};
            loadIfModified();

            std::uniform_real_distribution<double> distribution(0.95, 1.05);
            for (auto& x : state_.connector_state) {
                if (x.first.id == 0 || (x.first.connectorId && x.first.connectorId.value() == 0))
                    continue;

                auto amps = 48.0;
                if (x.second.limit.has_value())
                    amps = std::min(amps, x.second.limit.value());
                if (station_limit_.has_value())
                    amps = std::min(amps, station_limit_.value());

                x.second.amps = std::round((amps*distribution(random_engine_))*100.0)/100.0;
                x.second.volts = std::round((220.0*distribution(random_engine_))*100.0)/100.0;

                auto const now = platform_->steadyClockNow();
                if (x.second.last_update.has_value()) {
                    auto const delta = now - x.second.last_update.value();
                    auto const delta_hours = delta/(1000.0 * 3600.0);
                    x.second.watt_hours += x.second.amps*x.second.volts*delta_hours;
                }

                x.second.last_update = now;
            }
        }

        template<typename Visitor>
        void updateState(Visitor&& visitor) {
            std::lock_guard lock {mutex_};
            loadIfModified();
            visitor(state_);
            saveIfModified();
        }

    public:
        charger::StationMetadata getStationMetadata() override {
            std::lock_guard lock {mutex_};
            loadIfModified();

            return state_.station_metadata;
        }

        [[nodiscard]] std::map<ocpp2_0::EVSEType, charger::EvseMetadata> getEvseMetadata() override {
            std::lock_guard lock {mutex_};
            loadIfModified();

            return state_.evse_metadata;
        }

        [[nodiscard]] std::map<ocpp2_0::EVSEType, charger::ConnectorMetadata> getConnectorMetadata() override {
            std::lock_guard lock {mutex_};
            loadIfModified();

            return state_.connector_metadata;
        }

        std::optional<charger::ConnectorStatus> pollConnectorStatus(const ocpp2_0::EVSEType &evse) override {
            std::lock_guard lock {mutex_};
            loadIfModified();

            if (state_.connector_metadata.find(evse) == state_.connector_metadata.end())
                return std::nullopt;

            auto const& state = state_.connector_state[evse];
            return charger::ConnectorStatus {
                    state.connector_available,
                    state.vehicle_connected,
                    state.charging_enabled,
                    state.suspended_by_vehicle,
                    state.suspended_by_charger,
                    state.watt_hours,
                    std::nullopt
            };
        }

        std::vector<ocpp1_6::SampledValue> pollMeterValues1_6(const std::optional<ocpp2_0::EVSEType> &evse) override {
            std::lock_guard lock {mutex_};
            loadIfModified();

            // Note: not supporting station level meter values here
            if (!evse.has_value())
                return {};
            if (state_.connector_metadata.find(evse.value()) == state_.connector_metadata.end())
                return {};

            auto const& state = state_.connector_state[evse.value()];
            return {
                    ocpp1_6::SampledValue {
                            std::to_string(roundTo(state.watt_hours, 1)),
                            std::nullopt,
                            std::nullopt,
                            ocpp1_6::Measurand::kEnergyActiveImportRegister,
                            std::nullopt,
                            std::nullopt,
                            ocpp1_6::UnitOfMeasure::kWattHours
                    },
                    ocpp1_6::SampledValue {
                            std::to_string(roundTo(state.amps, 1)),
                            std::nullopt,
                            std::nullopt,
                            ocpp1_6::Measurand::kCurrentImport,
                            std::nullopt,
                            std::nullopt,
                            ocpp1_6::UnitOfMeasure::kAmps
                    },
                    ocpp1_6::SampledValue {
                            std::to_string(roundTo(state.volts, 1)),
                            std::nullopt,
                            std::nullopt,
                            ocpp1_6::Measurand::kVoltage,
                            std::nullopt,
                            std::nullopt,
                            ocpp1_6::UnitOfMeasure::kVolts
                    }
            };
        }

        std::vector<ocpp2_0::SampledValueType>
        pollMeterValues2_0(const std::optional<ocpp2_0::EVSEType> &evse) override {
            std::lock_guard lock {mutex_};
            loadIfModified();

            // Note: not supporting station level meter values here
            if (!evse.has_value())
                return {};
            if (state_.connector_metadata.find(evse.value()) == state_.connector_metadata.end())
                return {};

            auto const& state = state_.connector_state[evse.value()];
            return {
                    ocpp2_0::SampledValueType {
                            roundTo(state.watt_hours, 1),
                            std::nullopt,
                            ocpp2_0::MeasurandEnumType::kEnergy_Active_Import_Register,
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::UnitOfMeasureType {"Wh"}
                    },
                    ocpp2_0::SampledValueType {
                            roundTo(state.amps, 1),
                            std::nullopt,
                            ocpp2_0::MeasurandEnumType::kCurrent_Import,
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::UnitOfMeasureType {"A"}
                    },
                    ocpp2_0::SampledValueType {
                            roundTo(state.volts, 1),
                            std::nullopt,
                            ocpp2_0::MeasurandEnumType::kVoltage,
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            ocpp2_0::UnitOfMeasureType {"V"}
                    }
            };
        }

        void setChargingEnabled(const ocpp2_0::EVSEType &evse, bool value) override {
            std::lock_guard lock {mutex_};
            loadIfModified();

            if (state_.connector_metadata.find(evse) == state_.connector_metadata.end())
                return;

            state_.connector_state[evse].charging_enabled = value;
        }

        void setActiveChargePointMaxProfiles(std::vector<schedule_type1_6> const& active_schedules) override {
            std::lock_guard lock {mutex_};
            loadIfModified();

            // Note: ignoring phases
            std::optional<double> limit;
            for (auto const& x : active_schedules) {
                // Note: this should be handled here or the station should only accept a specific rate type
                if (x.first.csChargingProfiles.chargingSchedule.chargingRateUnit != ocpp1_6::ChargingRateUnitType::kA)
                    continue;

                if (!limit.has_value()) {
                    limit = x.second.limit;
                } else {
                    limit = std::min(limit.value(), x.second.limit);
                }
            }

            station_limit_ = limit;
        }

        void setActiveChargePointMaxProfiles(std::vector<schedule_type2_0> const& active_schedules) override {
            std::lock_guard lock {mutex_};
            loadIfModified();

            // Note: ignoring phases
            std::optional<double> limit;
            for (auto const& x : active_schedules) {
                // Note: profiles with multiple schedules are for ISO 15118 and aren't supported at the moment
                if (x.first.chargingProfile.chargingSchedule.size() != 1)
                    continue;

                auto const& schedule = x.first.chargingProfile.chargingSchedule.front();
                if (schedule.chargingRateUnit != ocpp2_0::ChargingRateUnitEnumType::kA)
                    continue;

                if (!limit.has_value()) {
                    limit = x.second.limit;
                } else {
                    limit = std::min(limit.value(), x.second.limit);
                }
            }

            station_limit_ = limit;
        }

        void setActiveEvseProfiles(int evse_id, std::vector<schedule_type1_6> const& active_schedules) override {
            std::lock_guard lock {mutex_};
            loadIfModified();

            // Note: ignoring phases
            std::optional<double> limit;
            for (auto const& x : active_schedules) {
                // Note: this should be handled here or the station should only accept a specific rate type
                if (x.first.csChargingProfiles.chargingSchedule.chargingRateUnit != ocpp1_6::ChargingRateUnitType::kA)
                    continue;

                if (!limit.has_value()) {
                    limit = x.second.limit;
                } else {
                    limit = std::min(limit.value(), x.second.limit);
                }
            }

            for (auto& x : state_.connector_state) {
                if (x.first.id != evse_id)
                    continue;

                x.second.limit = limit;
            }
        }

        void setActiveEvseProfiles(int evse_id, std::vector<schedule_type2_0> const& active_schedules) override {
            std::lock_guard lock {mutex_};
            loadIfModified();

            // Note: ignoring phases
            std::optional<double> limit;
            for (auto const& x : active_schedules) {
                // Note: profiles with multiple schedules are for ISO 15118 and aren't supported at the moment
                if (x.first.chargingProfile.chargingSchedule.size() != 1)
                    continue;

                auto const& schedule = x.first.chargingProfile.chargingSchedule.front();
                if (schedule.chargingRateUnit != ocpp2_0::ChargingRateUnitEnumType::kA)
                    continue;

                if (!limit.has_value()) {
                    limit = x.second.limit;
                } else {
                    limit = std::min(limit.value(), x.second.limit);
                }
            }

            for (auto& x : state_.connector_state) {
                if (x.first.id != evse_id)
                    continue;

                x.second.limit = limit;
            }
        }

        [[nodiscard]] std::optional<ocpp1_6::IdToken> readToken1_6() override {
            std::lock_guard lock {mutex_};
            if (!rfid_action_.has_value())
                return std::nullopt;
            if (platform_->steadyClockNow() - rfid_action_->first >= 0) {
                rfid_action_ = std::nullopt;
                return std::nullopt;
            }

            return rfid_action_->second.first;
        }

        [[nodiscard]] std::optional<ocpp2_0::IdTokenType> readToken2_0() override {
            std::lock_guard lock {mutex_};
            if (!rfid_action_.has_value())
                return std::nullopt;
            if (platform_->steadyClockNow() - rfid_action_->first >= 0) {
                rfid_action_ = std::nullopt;
                return std::nullopt;
            }

            return rfid_action_->second.second;
        }

    private:
        void loadIfModified() {
            auto const next_saved_state = saved_state_->getValue();
            if (last_saved_state_ == next_saved_state)
                return;

            auto const data = read_json_from_string<detail::SimulatorState>(next_saved_state);
            if (!data.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed reading saved state: " << next_saved_state;
            } else {
                state_ = data.value();
            }

            last_saved_state_ = next_saved_state;
        }

        void saveIfModified() {
            auto const next_saved_state = write_json_to_string(state_);
            if (last_saved_state_ == next_saved_state)
                return;

            saved_state_->setValue(next_saved_state);
            last_saved_state_ = next_saved_state;
        }

        static double roundTo(double value, int decimal_places) {
            auto const factor = (double)std::pow(10.0, decimal_places);
            return std::round(value/factor) * factor;
        }

    private:
        std::shared_ptr<PlatformInterface> platform_;
        std::shared_ptr<Settings> settings_;
        std::default_random_engine random_engine_;

        std::mutex mutex_ {};
        std::optional<double> station_limit_ = std::nullopt;
        std::optional<std::pair<SteadyPointMillis, std::pair<ocpp1_6::IdToken, ocpp2_0::IdTokenType>>> rfid_action_;

        detail::SimulatorState state_;
        std::shared_ptr<SettingString> saved_state_;
        std::string last_saved_state_;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_STATION_TEST_ESP32_H
