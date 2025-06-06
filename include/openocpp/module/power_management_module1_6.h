#ifndef CHARGELAB_OPEN_FIRMWARE_POWER_MANAGEMENT_MODULE1_6_H
#define CHARGELAB_OPEN_FIRMWARE_POWER_MANAGEMENT_MODULE1_6_H

#include "openocpp/module/common_templates.h"
#include "openocpp/interface/component/system_interface.h"
#include "openocpp/interface/station_interface.h"
#include "openocpp/common/settings.h"
#include "openocpp/common/compressed_journal.h"
#include "openocpp/helpers/set.h"
#include <queue>

namespace chargelab {
    class TransactionModule1_6;

    namespace detail {
        struct SchedulePeriod {
            std::int64_t startPeriod;  // exclusive, in seconds
            std::int64_t endTime;  // inclusive, for last period it could be INT64_MAX, in seconds
            double limit;
            int stackLevel;
            ocpp1_6::ChargingProfilePurposeType purpose;
            int chargingProfileId; // which charging profile is used for this period
            CHARGELAB_JSON_INTRUSIVE(SchedulePeriod, startPeriod, endTime, limit, stackLevel, purpose, chargingProfileId)
        };

        struct JournalUpdate1_6 {
            std::optional<ocpp1_6::SetChargingProfileReq> setChargingProfileReq = std::nullopt;
            std::optional<ocpp1_6::ClearChargingProfileReq> clearChargingProfileReq = std::nullopt;
            CHARGELAB_JSON_INTRUSIVE(JournalUpdate1_6, setChargingProfileReq, clearChargingProfileReq)
        };
    }

    // Note: this module treats connector 0 as a separate physical connector. Profiles assigned to connector ID 0 are
    // passed as a schedule to connector 0, not to each physical connector. This is intended to allow the station to
    // define a suitable load balancing mechanism when more than one connector is present.
    class PowerManagementModule1_6 : public ServiceStateful1_6 {
        static constexpr int kCriticalWriteCreditsInitial = 100;
        static constexpr std::int64_t kSecondsPerDay = 24 * 60 * 60;

        static constexpr int kMillisInDay = 1000*60*60*24;
        static constexpr int kMillisInWeek = kMillisInDay * 7;
        static constexpr int kJournalCapacityReportFrequencySeconds = 10;
        static constexpr int kFlashWriteCapacityOnBoot = 1024;
        static constexpr int kRequiredFlashLifetimeYears = 10;
        static constexpr double kRequiredFlashLifetimeMillis = kRequiredFlashLifetimeYears * (double)kMillisInDay * 365;
        static constexpr int kFlashLifetimeWriteCycles = 100000;

        friend class TransactionModule1_6;
    public:
        PowerManagementModule1_6(
                std::shared_ptr<Settings> settings,
                std::shared_ptr<SystemInterface> system,
                std::shared_ptr<StationInterface> station,
                std::unique_ptr<FlashBlockInterface> storage
        )
                : settings_(std::move(settings)),
                  system_(std::move(system)),
                  station_(std::move(station)),
                  journal_(std::move(storage), "1.6-1")
        {
            start_timestamp_ = system_->steadyClockNow();
            // Re-apply each of the items in the journal to recover the state
            CHARGELAB_LOG_MESSAGE(info) << "Re-applying saved 1.6 charging profiles";
            journal_.visit([&](std::string_view const& text, detail::JournalUpdate1_6 const& update) {
                {
                    // Note: sleeping briefly to avoid triggering the watchdog for large journals
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(1ms);
                }

                if (update.setChargingProfileReq.has_value()) {
                    auto const &request = update.setChargingProfileReq.value();

                    auto response = onSetChargingProfileReqInternal(request, false);
                    if (!response.has_value()) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Persisted profile was not accepted - null response: "
                                                       << text;
                    } else if (std::holds_alternative<ocpp1_6::CallError>(response.value())) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Persisted profile was not accepted - error response: "
                                                       << text;
                        CHARGELAB_LOG_MESSAGE(warning) << "Error response was: "
                                                       << std::get<ocpp1_6::CallError>(response.value());
                    } else if (std::holds_alternative<ocpp1_6::SetChargingProfileRsp>(response.value())) {
                        auto const &value = std::get<ocpp1_6::SetChargingProfileRsp>(response.value());
                        if (value.status != ocpp1_6::ChargingProfileStatus::kAccepted) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Persisted profile was not accepted - not accepted: "
                                                           << text;
                            CHARGELAB_LOG_MESSAGE(warning) << "Response was: " << value;
                        }
                    }
                }

                if (update.clearChargingProfileReq.has_value()) {
                    auto const& request = update.clearChargingProfileReq.value();

                    auto response = onClearChargingProfileReqInternal(request, false);
                    if (!response.has_value()) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Persisted clear profile request was not accepted - null response: " << text;
                    } else if (std::holds_alternative<ocpp1_6::CallError>(response.value())) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Persisted clear profile request was not accepted - error response: " << text;
                        CHARGELAB_LOG_MESSAGE(warning) << "Error response was: " << std::get<ocpp1_6::CallError>(response.value());
                    } else if (std::holds_alternative<ocpp1_6::ClearChargingProfileRsp>(response.value())) {
                        auto const& value = std::get<ocpp1_6::ClearChargingProfileRsp>(response.value());
                        if (value.status != ocpp1_6::ClearChargingProfileStatus::kAccepted) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Persisted clear profile request was not accepted - not accepted: " << text;
                            CHARGELAB_LOG_MESSAGE(warning) << "Response was: " << value;
                        }
                    }
                }
            });

            CHARGELAB_LOG_MESSAGE(info) << "final charging profiles: " << charging_profiles_;
        }

        ~PowerManagementModule1_6() override {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting PowerManagementModule1_6";
        }

    private:
        void onActiveTransactionStarted(int connector_id, std::optional<ocpp1_6::ChargingProfile> transaction_profile) {
            // As per 3.13.1. if a TxProfile is present in the RemoteStartTransaction request it overrides default
            // profile present, however it's not clear what this is intended to mean when the TxProfile expires or when
            // TxDefaultProfiles are updated within a transaction. Here we'll be adopting the conventions in OCPP 2.0.1
            // to resolve this type of ambiguity; see K.3.5. of the OCPP 2.0.1: Part 2 - Specification.
            if (transaction_profile.has_value()) {
                charging_profiles_.resize(charging_profiles_.size()+1);
                charging_profiles_.back() = ocpp1_6::SetChargingProfileReq {
                    connector_id,
                    transaction_profile.value()
                };
                publishProfileUpdates();
            }
        }

        void onActiveTransactionIdAssigned(int connector_id, int transaction_id) {            
            /*
                If there is no transaction active on the connector specified in a charging profile of type TxProfile, 
                then the Charge Point SHALL discard it and return an error status in SetChargingProfile.conf.
            */
            transaction_start_times_[connector_id] = system_->systemClockNow();
            active_transactions_[connector_id] = transaction_id;
        }

        void onActiveTransactionFinished(int connector_id) {
            charging_profiles_.erase(
                    std::remove_if(
                            charging_profiles_.begin(),
                            charging_profiles_.end(),
                            [&](ocpp1_6::SetChargingProfileReq const& profile) {
                                if (profile.csChargingProfiles.chargingProfilePurpose != ocpp1_6::ChargingProfilePurposeType::kTxProfile)
                                    return false;
                                if (profile.connectorId != connector_id)
                                    return false;

                                return true;
                            }
                    ),
                    charging_profiles_.end()
            );

            transaction_start_times_[connector_id] = std::nullopt;
            auto it = active_transactions_.find(connector_id);
            if (it != active_transactions_.end()) {
                active_transactions_.erase(it);
            }

            publishProfileUpdates();
        }

        std::optional<ocpp1_6::ResponseToRequest <ocpp1_6::SetChargingProfileRsp>>
        onSetChargingProfileReq(const ocpp1_6::SetChargingProfileReq &req) override {
            return onSetChargingProfileReqInternal(req, true);
        }

        std::optional<ocpp1_6::ResponseToRequest <ocpp1_6::SetChargingProfileRsp>>
        onSetChargingProfileReqInternal(const ocpp1_6::SetChargingProfileReq &req, bool persist_to_journal) {
            if (req.connectorId != 0 && !station_->lookupConnectorId1_6(req.connectorId).has_value()) {
                return ocpp1_6::CallError {
                        ocpp1_6::ErrorCode::kPropertyConstraintViolation,
                        "ConnectorId does not exist",
                        common::RawJson::empty_object()
                };
            }

            switch (req.csChargingProfiles.chargingProfilePurpose) {
                case ocpp1_6::ChargingProfilePurposeType::kTxProfile:
                    // Note: the Transaction modules are expected to reject profiles  with invalid transaction IDs and
                    // clear profiles when transactions complete.
                    if (req.connectorId == 0) {
                        return ocpp1_6::CallError {
                                ocpp1_6::ErrorCode::kPropertyConstraintViolation,
                                "TxProfile must specify a connector ID",
                                common::RawJson::empty_object()
                        };
                    }
                    if (!req.csChargingProfiles.transactionId.has_value()) {
                        return ocpp1_6::CallError {
                                ocpp1_6::ErrorCode::kFormationViolation,
                                "TxProfile must specify transactionId",
                                common::RawJson::empty_object()
                        };
                    }
                    // There must be an ongoing transaction with the same transactionId on this connector
                    if (active_transactions_[req.connectorId] != req.csChargingProfiles.transactionId) {
                        return ocpp1_6::SetChargingProfileRsp {ocpp1_6::ChargingProfileStatus::kRejected};
                    }
                    break;

                case ocpp1_6::ChargingProfilePurposeType::kTxDefaultProfile:
                    if (req.csChargingProfiles.transactionId.has_value()) {
                        return ocpp1_6::CallError {
                                ocpp1_6::ErrorCode::kFormationViolation,
                                "TxDefaultProfile must not specify transactionId",
                                common::RawJson::empty_object()
                        };
                    }
                    break;

                case ocpp1_6::ChargingProfilePurposeType::kChargePointMaxProfile:
                    if (req.connectorId != 0) {
                        return ocpp1_6::CallError {
                                ocpp1_6::ErrorCode::kPropertyConstraintViolation,
                                "ChargePointMaxProfile may only be set for connectorId 0",
                                common::RawJson::empty_object()
                        };
                    }
                    if (req.csChargingProfiles.transactionId.has_value()) {
                        return ocpp1_6::CallError {
                                ocpp1_6::ErrorCode::kFormationViolation,
                                "ChargePointMaxProfile must not specify transactionId",
                                common::RawJson::empty_object()
                        };
                    }
                    break;

                default:
                    return ocpp1_6::CallError {
                            ocpp1_6::ErrorCode::kPropertyConstraintViolation,
                            "Unknown charging profile purpose",
                            common::RawJson::empty_object()
                    };
            }

            // check the charging rate unit
            auto const& schedule = req.csChargingProfiles.chargingSchedule;
            if (schedule.chargingRateUnit == ocpp1_6::ChargingRateUnitType::kValueNotFoundInEnum) {
                return ocpp1_6::CallError {
                        ocpp1_6::ErrorCode::kPropertyConstraintViolation,
                        "Unknown charging rate unit",
                        common::RawJson::empty_object()
                };
            }

            auto const& schedule_periods = schedule.chargingSchedulePeriod;
            if (schedule_periods.empty()) {
                return ocpp1_6::CallError {
                        ocpp1_6::ErrorCode::kPropertyConstraintViolation,
                        "No schedule periods in received charging profile",
                        common::RawJson::empty_object()
                };
            }

            if (req.csChargingProfiles.stackLevel > settings_->ChargeProfileMaxStackLevel.getValue()) {
                return ocpp1_6::CallError {
                        ocpp1_6::ErrorCode::kPropertyConstraintViolation,
                        "Stack level exceeded the limit: " + std::to_string(settings_->ChargeProfileMaxStackLevel.getValue()),
                        common::RawJson::empty_object()
                };
            }

            // If the charging profile already exists there's nothing left to do
            for (auto& x : charging_profiles_) {
                if (x == req)
                    return ocpp1_6::SetChargingProfileRsp {ocpp1_6::ChargingProfileStatus::kAccepted};
            }

            // Check if we've exceeded the limits (number of writes) and must fail
            if (persist_to_journal) {
                if (!allowJournalUpdate())
                    return ocpp1_6::SetChargingProfileRsp{ocpp1_6::ChargingProfileStatus::kRejected};
            }

            // Note: to comply with 3.13.2 all profiles matching the following need to be removed:
            // - The same profile purpose and stack level
            // - The same profile ID (this is assumed to be the intent here)
            auto old_profiles = charging_profiles_;

            charging_profiles_.erase(
                    std::remove_if(
                            charging_profiles_.begin(),
                            charging_profiles_.end(),
                            [&](ocpp1_6::SetChargingProfileReq const& profile) {
                                auto const& lhs = profile.csChargingProfiles;
                                auto const& rhs = req.csChargingProfiles;
                                if (lhs.chargingProfileId == rhs.chargingProfileId)
                                    return true;
                                if (lhs.chargingProfilePurpose == rhs.chargingProfilePurpose && lhs.stackLevel == rhs.stackLevel)
                                    return true;

                                return false;
                            }
                    ),
                    charging_profiles_.end()
            );

            if ((int)charging_profiles_.size() >= settings_->MaxChargingProfilesInstalled.getValue()) {
                charging_profiles_ = old_profiles; // Restore to the previous profiles
                return ocpp1_6::CallError {
                        ocpp1_6::ErrorCode::kPropertyConstraintViolation,
                        "Installed profiles reached the limit of " + std::to_string(settings_->MaxChargingProfilesInstalled.getValue()),
                        common::RawJson::empty_object()
                };
            }

            if (persist_to_journal && req.csChargingProfiles.chargingProfilePurpose != ocpp1_6::ChargingProfilePurposeType::kTxProfile) {
                if (!journal_.addUpdate(chargingProfilesToState(), detail::JournalUpdate1_6{req})) {
                    charging_profiles_ = old_profiles; // Restore to the previous profiles
                    return ocpp1_6::SetChargingProfileRsp{ocpp1_6::ChargingProfileStatus::kRejected};
                }
            }

            // Note: avoiding push_back to limit heap usage to active elements.
            charging_profiles_.resize(charging_profiles_.size()+1);
            charging_profiles_.back() = req;

            if (persist_to_journal) {
                publishProfileUpdates();
            }

            return ocpp1_6::SetChargingProfileRsp {ocpp1_6::ChargingProfileStatus::kAccepted};
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ClearChargingProfileRsp>>
        onClearChargingProfileReq(const ocpp1_6::ClearChargingProfileReq& req) override {
            return onClearChargingProfileReqInternal(req, true);
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::ClearChargingProfileRsp>>
        onClearChargingProfileReqInternal(const ocpp1_6::ClearChargingProfileReq& req, bool persist_to_journal) {
            if (req.stackLevel.has_value() && req.stackLevel.value() < 0) {
                return ocpp1_6::CallError {
                        ocpp1_6::ErrorCode::kPropertyConstraintViolation,
                        "Stack level is negative",
                        common::RawJson::empty_object()
                };
            }

            // TODO: Why? Does this make sense?
            if (req.connectorId.has_value() && req.connectorId.value() == 0) {
                return ocpp1_6::CallError {
                        ocpp1_6::ErrorCode::kNotSupported,
                        "Connector Id = 0 is not supported",
                        common::RawJson::empty_object()
                };
            }

            std::vector<ocpp1_6::SetChargingProfileReq> old_profiles;

            if (persist_to_journal) {
                old_profiles = charging_profiles_;
            }

            charging_profiles_.erase(
                    std::remove_if(
                            charging_profiles_.begin(),
                            charging_profiles_.end(),
                            [&](ocpp1_6::SetChargingProfileReq const& x) {
                                if (req.id.has_value()) {
                                    // If an ID is present match on it exclusively
                                    return req.id.value() == x.csChargingProfiles.chargingProfileId;
                                } else {
                                    // Otherwise match against the supplied parameters
                                    bool match = true;
                                    if (req.connectorId.has_value())
                                        match &= req.connectorId.value() == x.connectorId;
                                    if (req.chargingProfilePurpose.has_value())
                                        match &= req.chargingProfilePurpose.value() == x.csChargingProfiles.chargingProfilePurpose;
                                    if (req.stackLevel.has_value())
                                        match &= req.stackLevel.value() == x.csChargingProfiles.stackLevel;

                                    return match;
                                }
                            }
                    ),
                    charging_profiles_.end()
            );

            if (persist_to_journal && old_profiles.size() > charging_profiles_.size()) {
                if (allowJournalUpdate()) {
                    if (!journal_.addUpdate(chargingProfilesToState(),
                                            detail::JournalUpdate1_6{std::nullopt, {req}})) {
                        charging_profiles_ = old_profiles; // Restore to the previous profiles
                        return ocpp1_6::CallError {
                                ocpp1_6::ErrorCode::kNotSupported,
                                "Failed writing update to flash storage",
                                common::RawJson::empty_object()
                        };
                    }
                }
            }

            // Note: clear operations are not rate-limited
            publishProfileUpdates();

            return ocpp1_6::ClearChargingProfileRsp {ocpp1_6::ClearChargingProfileStatus::kAccepted};
        }


        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::GetCompositeScheduleRsp>>
        onGetCompositeScheduleReq(const ocpp1_6::GetCompositeScheduleReq& req) override {
            auto time_now = system_->systemClockNow();
            std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::GetCompositeScheduleRsp>> rsp;

            getCompositeSchedule(req, time_now,
                                        [&](std::vector<detail::SchedulePeriod> const& periods, int duration) {
                std::vector<ocpp1_6::ChargingSchedulePeriod> target(periods.size());

                std::transform(periods.begin(), periods.end(), target.begin(),
                               [=](detail::SchedulePeriod const &source) {
                                   return ocpp1_6::ChargingSchedulePeriod{ std::max(static_cast<std::int64_t>(0),
                                                                                    source.startPeriod - time_now/1000),
                                                                          source.limit, std::nullopt};
                               });
                if (duration < req.duration) {
                    // add a period with default device voltage as its limit
                    double connector_default_max_amps = 0;

                    for (auto const& x : station_->getConnectorMetadata()) {
                        if (x.second.connector_id1_6 == req.connectorId) {
                            connector_default_max_amps = x.second.power_max_amps;
                            break;
                        }
                    }

                    target.emplace_back(ocpp1_6::ChargingSchedulePeriod{
                            duration,
                            connector_default_max_amps,
                            std::nullopt
                    });

                    duration = req.duration;
                }

                rsp = std::make_optional(ocpp1_6::GetCompositeScheduleRsp{
                        ocpp1_6::GetCompositeScheduleStatus::kAccepted,
                        req.connectorId,
                        target.empty() ? std::nullopt : std::make_optional(ocpp1_6::DateTime(time_now)), // schedule stat from now
                        target.empty() ? std::nullopt : std::make_optional(ocpp1_6::ChargingSchedule {
                                duration,
                                ocpp1_6::DateTime(time_now),  // start the schedule right now
                                ocpp1_6::ChargingRateUnitType::kA,
                                std::move(target),
                                std::nullopt
                        })
                });
            });

            return rsp;
        }

        void runStep(ocpp1_6::OcppRemote&) override {
            // Remove expired profiles

            // TODO: Load charging profiles somewhere
            // TODO: Charging profiles shouldn't be applied until the clock is synchronized
            if (!charging_profile_applied_) {
                if (system_->isClockOutOfSync())
                    return;

                start_timestamp_ = system_->steadyClockNow();
                publishProfileUpdates();
                charging_profile_applied_ = true;
            }

            // Update the active profiles if there were changes
            if (next_profile_update_.has_value()) {
                if (system_->systemClockNow() - next_profile_update_.value() >= 0)
                    publishProfileUpdates();
            }

            // TODO: Support for offline overrides should be preserved for now
            // If no profiles applied, the default device maximum power should be applied.
        }

    private:
        void getCompositeSchedule(const ocpp1_6::GetCompositeScheduleReq& req, SystemTimeMillis const& time_now,
                                  std::function<void(std::vector<detail::SchedulePeriod> const& periods,
                                          int duration)> handler) const {
            // 1: Get connectorId relevant charging profiles
            std::vector<ocpp1_6::ChargingProfile> transactionProfiles; // profiles for transaction and default transaction profiles
            std::vector<ocpp1_6::ChargingProfile> maxProfiles; // max profiles
            
            for (auto const& profile : charging_profiles_) {
                if (profile.connectorId == 0 || profile.connectorId == req.connectorId) {
                    if (profile.csChargingProfiles.chargingProfilePurpose == ocpp1_6::ChargingProfilePurposeType::kChargePointMaxProfile) {
                        maxProfiles.push_back(profile.csChargingProfiles);
                    } else {
                        transactionProfiles.push_back(profile.csChargingProfiles);
                    }
                }
            }

            // 2: Build all charging schedule periods for txProfiles and maxProfiles separately
            auto it = transaction_start_times_.find(req.connectorId);
            auto raw_transaction_schedule = generateUnorderedSchedule(transactionProfiles,
                req.duration, 
                time_now,
                it == transaction_start_times_.end() ? std::nullopt : it->second);
            auto transaction_schedule = generateCompositeSchedule(raw_transaction_schedule, time_now, false);
                         
            auto raw_max_schedule = generateUnorderedSchedule(maxProfiles,
                req.duration, 
                time_now,
                it == transaction_start_times_.end() ? std::nullopt : it->second);
            auto max_schedule = generateCompositeSchedule(raw_max_schedule, time_now, false);

            // 3: generate the composite schedule based on the combined schedules of the above schedule periods
            max_schedule.insert(max_schedule.end(), transaction_schedule.begin(), transaction_schedule.end());    
            auto composite_schedule = generateCompositeSchedule(max_schedule, time_now, true);

            int duration = !composite_schedule.empty() ? std::min(composite_schedule.back().endTime - time_now/1000, static_cast<std::int64_t>(req.duration)) : 0;

            // 4: let the caller handle the result
            handler(composite_schedule, duration);
        }

        // Generate the final composite schedule but with real timestamps
        std::vector<detail::SchedulePeriod> generateCompositeSchedule(
                std::vector<detail::SchedulePeriod> const & raw_schedule,
                SystemTimeMillis const & time_now,
                bool transaction_max_mixed_schedules) const {

            std::int64_t time_now_seconds = time_now/1000;

            // Create the comparator for the priority_queue so that it works for our algorithm
            auto cmp = [](detail::SchedulePeriod const& a, detail::SchedulePeriod const& b) {
                if (a.startPeriod == b.startPeriod) {
                    if (a.purpose != b.purpose) {
                        // get element with bigger profile level first (make the maximum profile last!!! So it can check the limit not greater than the max value)
                        return a.purpose < b.purpose;
                    } else {
                        return a.stackLevel < b.stackLevel;  // get the bigger stack level first in case of MAX profile, so the right one comes first and lower stack level will be ignored
                    }
                } else {
                    return a.startPeriod > b.startPeriod;  // get element with smaller start value first
                }
            };

            // Put all the raw_schedule items into a priority_queue for handling
            std::priority_queue<detail::SchedulePeriod, std::vector<detail::SchedulePeriod>,
                decltype(cmp)> pq(std::make_move_iterator(raw_schedule.begin()), std::make_move_iterator(raw_schedule.end()), cmp);

            if (pq.empty()) {
                return { };
            }

            // handle all the periods in the priority queue
            std::vector<detail::SchedulePeriod> composite;
            std::optional<detail::SchedulePeriod> last = std::nullopt;

            while (!pq.empty()) {
                auto current = pq.top();
                pq.pop();
                if (current.startPeriod == current.endTime) {
                    continue;
                }
                if (last) {
                    if (last->startPeriod >= last->endTime) {
                        last = std::nullopt;
                    }
                }

                if (!last) {
                    last = std::move(current);
                    continue;
                }

                if (current.startPeriod > last->startPeriod) {
                    // Now is a chance to finish the last node
                    auto last2 = last.value();
                    last2.endTime = current.startPeriod;
                    if (last2.endTime > time_now_seconds) {
                        composite.push_back(last2);
                    }

                    last->startPeriod = current.startPeriod;
                    if (last->startPeriod != last->endTime) {
                        pq.push(std::move(last.value()));
                        last = std::nullopt; // start over
                    }
                    pq.push(current);
                    continue;
                }

                // Save part of current if it is longer than last
                if (current.endTime > last->endTime) {
                    auto current2 = current;
                    current2.startPeriod = last->endTime;
                    pq.push(current2); // push back the current2 for future handling
                    current.endTime = last->endTime;
                } 
                
                // The logic below is for handling maxProfile and TxProfile mixed schedules
                if (transaction_max_mixed_schedules) {
                    if (current.endTime < last->endTime) {
                        auto last2 = last.value();
                        last2.startPeriod = current.endTime;
                        pq.push(last2); // push back the last2 for future process
                        last->endTime = current.endTime;
                    }

                    if (current.purpose != last->purpose) {
                        if (last->purpose == ocpp1_6::ChargingProfilePurposeType::kChargePointMaxProfile) {
                            if (last->limit >= current.limit) {
                                last = current; // current should be selected
                            }
                        } else {  // current is max profile
                            if (last->limit > current.limit) {
                                last = current; // current should be selected
                            }
                        }
                    }
                }
            }

            if (last.has_value()) {
                composite.push_back(last.value());
            }

            auto it = composite.begin();
            while (it != composite.end()) {
                if (it->endTime <= time_now_seconds) {
                    it = composite.erase(it);
                    continue;
                }
                auto it2 = it + 1;
                if (it2 == composite.end()) {
                    break;
                }
                if (it2->limit == it->limit) {
                    it->endTime = it2->endTime;
                    composite.erase(it2);
                } else {
                    it = it2;
                }
            }

            return composite;
        }

        // Generate initial schedule periods from the profiles without checking their priority, e.g. stackLevel, ChargingProfilePurposeType
        std::vector<detail::SchedulePeriod> generateUnorderedSchedule(
                std::vector<ocpp1_6::ChargingProfile> const & profiles,
                int requested_duration_seconds,
                SystemTimeMillis const & time_now,
                std::optional<SystemTimeMillis> const& transaction_start_time) const {
            std::vector<detail::SchedulePeriod> merged;
            std::int64_t time_now_seconds = time_now/1000;
            std::int64_t end_time = time_now_seconds + requested_duration_seconds;

            for (auto const& profile : profiles) {
                if (profile.validFrom && profile.validFrom.value().getTimestamp() &&
                    (profile.validFrom.value().getTimestamp().value() - (time_now + requested_duration_seconds*1000))/1000 >= 0) {
                    continue;
                }
                if (profile.validTo && profile.validTo.value().getTimestamp() && profile.validTo.value().getTimestamp().value()/1000 <= time_now_seconds) {
                    continue;
                }

                auto adjusted_end_time = end_time;

                if (profile.validTo && profile.validTo.value().getTimestamp()) {
                    adjusted_end_time = std::min(adjusted_end_time, profile.validTo.value().getTimestamp().value()/1000);
                }

                if (profile.chargingProfileKind == ocpp1_6::ChargingProfileKindType::kRecurring && profile.recurrencyKind.has_value()) {
                    auto start_time = time_now_seconds;
                    while (start_time < end_time) {
                        auto periods = generateSchedulePeriods(profile, start_time, adjusted_end_time, transaction_start_time);
                        merged.insert(merged.begin(), periods.begin(), periods.end());
                        start_time += profile.recurrencyKind.value() == ocpp1_6::RecurrencyKindType::kDaily ? kSecondsPerDay : 7 * kSecondsPerDay;
                    }
                } else {
                    auto periods = generateSchedulePeriods(profile, time_now_seconds, adjusted_end_time, transaction_start_time);
                    merged.insert(merged.begin(), periods.begin(), periods.end());
                }
            }

            return merged;
        }

        // Generate schedule periods and add them to the periods parameter, each period has a start and end time
        std::vector<detail::SchedulePeriod> generateSchedulePeriods(
            ocpp1_6::ChargingProfile const &profile,
            std::int64_t start_time_now,
            std::int64_t end_time,
            std::optional<SystemTimeMillis> const& transaction_start_time // for relative profile kind
            ) const {
            auto startSchedule = profile.chargingSchedule.startSchedule;
            std::int64_t base_start_time = startSchedule.has_value() && startSchedule.value().getTimestamp().has_value() ?
                                            startSchedule->getTimestamp().value()/1000 : start_time_now;

            std::vector<detail::SchedulePeriod> periods;

            for (auto const& period : profile.chargingSchedule.chargingSchedulePeriod) {
                std::int64_t absolute_start_time = base_start_time + period.startPeriod;

                if (profile.chargingProfileKind == ocpp1_6::ChargingProfileKindType::kRelative) {
                    // Use transaction_start_time if it has value as the time start point
                    // TODO: else use the absolute time or ignore this profile ?
                    if (transaction_start_time.has_value()) {
                        absolute_start_time = transaction_start_time.value()/1000 + period.startPeriod;
                    }
                }

                // Only add periods within the requested duration
                if (absolute_start_time < end_time) {
                    if (profile.chargingSchedule.duration.has_value()) {
                        if (absolute_start_time - base_start_time < profile.chargingSchedule.duration.value()) {
                            // Pre-set the endTime as the min value of end_time and base_start_time + profile.chargingSchedule.duration.value()
                            // if not the last period, it will be overridden by the next time's start time
                            if (!periods.empty()) {
                                periods.back().endTime = absolute_start_time;
                            }
                            periods.emplace_back(detail::SchedulePeriod{ absolute_start_time,
                                                 std::min(base_start_time + profile.chargingSchedule.duration.value(), end_time),
                                                 period.limit,
                                                 profile.stackLevel,
                                                 profile.chargingProfilePurpose,
                                                 profile.chargingProfileId});
                        } else {
                            break;
                        }
                    } else {
                        // Set endTime with INT64_MAX so the last period will last forever
                        if (!periods.empty()) {
                            periods.back().endTime = absolute_start_time;
                        }

                        periods.emplace_back(detail::SchedulePeriod{ absolute_start_time, INT64_MAX, period.limit, profile.stackLevel,
                                             profile.chargingProfilePurpose, profile.chargingProfileId});
                    }
                }
            }

            return periods;
        }

        void updateActiveSchedules() {
            auto const now = system_->systemClockNow();
            auto const time_now_seconds = now/1000;

            const int one_hour_duration = 60*60; // 1 hour
            std::int64_t shortest_duration = one_hour_duration;

            std::set<int> connector_ids;
            connector_ids.insert(0);
            for (auto const& x : station_->getConnectorMetadata())
                connector_ids.insert(x.second.connector_id1_6);

            for (auto const & connector : connector_ids) {
                getCompositeSchedule(ocpp1_6::GetCompositeScheduleReq {
                    connector,
                    one_hour_duration,
                    ocpp1_6::ChargingRateUnitType::kA },
                                     now,
                                     [&](std::vector<detail::SchedulePeriod> const& periods, int duration) {
                    for (auto const& p : periods) {
                        if (p.endTime > time_now_seconds) {
                            shortest_duration = std::min(shortest_duration, p.endTime - std::max(p.startPeriod, time_now_seconds));
                            std::vector<StationInterface::schedule_type1_6> active_schedules;

                            auto it = std::find_if(charging_profiles_.begin(), charging_profiles_.end(), [=](auto const& profile) {
                                return profile.csChargingProfiles.chargingProfileId == p.chargingProfileId;
                            });

                            active_schedules.push_back({*it, ocpp1_6::ChargingSchedulePeriod {
                                p.startPeriod - time_now_seconds, p.limit}});

                            if (connector == 0) {
                                station_->setActiveChargePointMaxProfiles(active_schedules);
                            } else {
                                station_->setActiveEvseProfiles(connector, active_schedules);
                            }
                            break;
                        }
                    }
                });
            }
            //
            auto time_now2 = static_cast<std::int64_t>(now) + shortest_duration * 1000;
            next_profile_update_ = static_cast<SystemTimeMillis>(time_now2);
        }

#if 0
        void updateActiveSchedules() {
            // Note: the order is important here - the loop below breaks if TxProfile is valid at this point in time to
            // exclude the TxDefaultProfile.
            auto const profile_types = std::array<ocpp1_6::ChargingProfilePurposeType, 3> {
                    ocpp1_6::ChargingProfilePurposeType::kChargePointMaxProfile,
                    ocpp1_6::ChargingProfilePurposeType::kTxProfile,
                    ocpp1_6::ChargingProfilePurposeType::kTxDefaultProfile
            };

            auto const now = system_->systemClockNow();
            std::optional<SystemTimeMillis> next_update;

            std::set<int> evse_ids;
            evse_ids.insert(0);
            for (auto const& x : station_->getConnectorMetadata())
                evse_ids.insert(x.second.connector_id1_6);

            for (auto& evse_id : evse_ids) {
                std::vector<StationInterface::schedule_type1_6> active_schedules;
                active_schedules.reserve(2);

                for (auto const& type : profile_types) {
                    auto profile = getProfileAtTime(evse_id, now, type);
                    bool valid_profile = false;
                    if (profile.first.has_value()) {
                        auto period = getPeriodAtTime(profile.first.value(), now);
                        if (period.has_value()) {
                            active_schedules.emplace_back(profile.first.value(), period.value());
                            valid_profile = true;
                        }
                    }

                    if (profile.second.has_value()) {
                        if (next_update.has_value()) {
                            next_update = std::min(next_update.value(), profile.second.value());
                        } else {
                            next_update = profile.second.value();
                        }
                    }

                    // Break to exclude TxDefaultProfile
                    if (type == ocpp1_6::ChargingProfilePurposeType::kTxProfile && valid_profile)
                        break;
                }

                if (evse_id == 0) {
                    station_->setActiveChargePointMaxProfiles(active_schedules);
                } else {
                    station_->setActiveEvseProfiles(evse_id, active_schedules);
                }
            }

            next_profile_update_ = next_update;
        }
#endif

        bool persistChargingProfile(const ocpp1_6::SetChargingProfileReq &req) {
            (void)req;
//            if (req.csChargingProfiles.chargingProfilePurpose == ocpp1_6::ChargingProfilePurposeType::kTxProfile) {
//                // Note: TxProfiles are never persisted
//                return false;
//            }
//
//            bool persist = true;
//            auto visitor = [&](auto const &value) {
//                if (string::ToInteger(value) == std::make_optional<int>(req.csChargingProfiles.chargingProfileId))
//                    persist = false;
//            };
//
//            // Note: even through "NonPersistentProfileIdsWithRestartProtection" may result is something hitting disk
//            // I'm ignoring that here for the purpose of defining which profiles need to be persisted. Those types of
//            // writes are throttled separately to limit worst case wear.
//            string::SplitVisitor(settings_->NonPersistentProfileIdsWithoutRestartProtection.getValue(), ",", visitor);
//            string::SplitVisitor(settings_->NonPersistentProfileIdsWithRestartProtection.getValue(), ",", visitor);
//
//            return persist;

            return true;
        }

        /*bool acceptChargingProfile(const ocpp1_6::SetChargingProfileReq &req) {
            // Accept any updates that won't require immediate persistence
            if (!persistChargingProfile(req))
                return true;

            // TODO: Otherwise rate-limit writes to disk?
            return true;
        }*/

        void publishProfileUpdates() {
            if (system_->isClockOutOfSync())
                return;
            updateActiveSchedules();
        }

        std::pair<std::optional<ocpp1_6::SetChargingProfileReq>, std::optional<SystemTimeMillis>> getProfileAtTime(
            int connectorId,
            SystemTimeMillis timestamp,
            ocpp1_6::ChargingProfilePurposeType profileType
        ) {
            std::optional<ocpp1_6::SetChargingProfileReq> active;
            for (auto const& profile : charging_profiles_) {
                // Note: connectorId is treated as a separate connector
                if (profile.connectorId != connectorId)
                    continue;
                if (profile.csChargingProfiles.chargingProfilePurpose != profileType)
                    continue;

                if (profile.csChargingProfiles.validFrom.has_value()) {
                    auto valid_from_ts = profile.csChargingProfiles.validFrom->getTimestamp();
                    if (!valid_from_ts.has_value()) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Charging profile has validFrom but the value wasn't convertable into a timestamp: "
                                                       << profile.csChargingProfiles.validFrom->getText();
                    } else if (timestamp < valid_from_ts.value()) {
                        // If the profile isn't active yet skip it
                        continue;
                    }
                }

                if (profile.csChargingProfiles.validTo.has_value()) {
                    auto valid_to_ts = profile.csChargingProfiles.validTo->getTimestamp();
                    if (!valid_to_ts.has_value()) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Charging profile has validTo but the value wasn't convertable into a timestamp: "
                                                       << profile.csChargingProfiles.validTo->getText();
                    } else if (timestamp >= valid_to_ts.value()) {
                        // If the profile is expired skip it
                        continue;
                    }
                }

                // Note: the transaction modules will add/remove transaction profiles as transactions are
                // started/stopped. We can ignore the transaction ID here and just assume any transaction profiles
                // present are applicable.

                if (!active.has_value() || profile.csChargingProfiles.stackLevel > active->csChargingProfiles.stackLevel)
                    active = profile;
            }

            // We want to find the next profile that will replace the active profile within it's validity period.
            std::optional<SystemTimeMillis> active_valid_to;
            if (active.has_value() && active->csChargingProfiles.validTo.has_value())
                active_valid_to = active->csChargingProfiles.validTo->getTimestamp();

            std::optional<ocpp1_6::SetChargingProfileReq> next;
            for (auto const& profile : charging_profiles_) {
                if (profile.connectorId != connectorId)
                    continue;
                if (profile.csChargingProfiles.chargingProfilePurpose != profileType)
                    continue;

                if (profile.csChargingProfiles.validFrom.has_value()) {
                    auto valid_from_ts = profile.csChargingProfiles.validFrom->getTimestamp();
                    if (!valid_from_ts.has_value()) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Charging profile has validFrom but the value wasn't convertable into a timestamp: "
                                                       << profile.csChargingProfiles.validFrom->getText();
                    } else if (valid_from_ts.value() <= timestamp) {
                        // Skip any of the profiles we would have considered above
                        continue;
                    } else if (active_valid_to.has_value() && active_valid_to.value() <= valid_from_ts.value()) {
                        // If this profile starts at or after the end of the current active profile skip it
                        continue;
                    }
                }

                if (profile.csChargingProfiles.validTo.has_value()) {
                    auto valid_to_ts = profile.csChargingProfiles.validTo->getTimestamp();
                    if (!valid_to_ts.has_value()) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Charging profile has validTo but the value wasn't convertable into a timestamp: "
                                                       << profile.csChargingProfiles.validTo->getText();
                    } else if (timestamp >= valid_to_ts.value()) {
                        // If the profile is expired skip it
                        continue;
                    }
                }

                if (!next.has_value() || profile.csChargingProfiles.stackLevel > next->csChargingProfiles.stackLevel)
                    next = profile;
            }

            std::optional<SystemTimeMillis> valid_until;
            if (next.has_value() && next->csChargingProfiles.validFrom.has_value()) {
                valid_until = next->csChargingProfiles.validFrom->getTimestamp();
            } else if (active.has_value() && active->csChargingProfiles.validTo.has_value()) {
                valid_until = active->csChargingProfiles.validTo->getTimestamp();
            }

            return std::make_pair(active, valid_until);
        }

        std::optional<ocpp1_6::ChargingSchedulePeriod> getPeriodAtTime(
                ocpp1_6::SetChargingProfileReq const& profile,
                SystemTimeMillis timestamp
        ) {
            if (profile.csChargingProfiles.validFrom.has_value()) {
                auto valid_from_ts = profile.csChargingProfiles.validFrom->getTimestamp();
                if (!valid_from_ts.has_value()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Charging profile has validFrom but the value wasn't convertable into a timestamp: "
                                                   << profile.csChargingProfiles.validFrom->getText();
                } else if (timestamp < valid_from_ts.value()) {
                    // This profile isn't active yet
                    return std::nullopt;
                }
            }

            if (profile.csChargingProfiles.validTo.has_value()) {
                auto valid_to_ts = profile.csChargingProfiles.validTo->getTimestamp();
                if (!valid_to_ts.has_value()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Charging profile has validTo but the value wasn't convertable into a timestamp: "
                                                   << profile.csChargingProfiles.validTo->getText();
                } else if (timestamp >= valid_to_ts.value()) {
                    // This profile is expired
                    return std::nullopt;
                }
            }

            // The schedule start must be set either at the time the SetChargingProfile request was received, or at the
            // time a transaction started.
            if (!profile.csChargingProfiles.chargingSchedule.startSchedule.has_value()) {
                CHARGELAB_LOG_MESSAGE(error) << "Charging profile missing startSchedule: " << profile;
                return std::nullopt;
            }

            // Schedule start
            SystemTimeMillis schedule_start;
            {
                auto schedule_start_ts = profile.csChargingProfiles.chargingSchedule.startSchedule->getTimestamp();
                if (!schedule_start_ts.has_value()) {
                    CHARGELAB_LOG_MESSAGE(error) << "Charging profile startSchedule not convertable to a timestamp: " << profile;
                    return std::nullopt;
                }

                schedule_start = schedule_start_ts.value();
            }

            if (profile.csChargingProfiles.chargingProfileKind == ocpp1_6::ChargingProfileKindType::kRecurring) {
                if (!profile.csChargingProfiles.recurrencyKind.has_value()) {
                    CHARGELAB_LOG_MESSAGE(error) << "Recurring charging profile missing recurrencyKind: " << profile;
                    return std::nullopt;
                }

                int recurrency_period_millis;
                switch (profile.csChargingProfiles.recurrencyKind.value()) {
                    case ocpp1_6::RecurrencyKindType::kDaily:
                        recurrency_period_millis = 1000 * 60 * 60 * 24;
                        break;

                    case ocpp1_6::RecurrencyKindType::kWeekly:
                        recurrency_period_millis = 1000 * 60 * 60 * 24 * 7;
                        break;

                    default:
                        CHARGELAB_LOG_MESSAGE(error) << "Bad charging profile recurrencyKind: " << profile;
                        return std::nullopt;
                }

                auto delta = timestamp - schedule_start;
                auto periods = delta / recurrency_period_millis;
                if (delta < 0)
                    periods -= 1;

                schedule_start = static_cast<SystemTimeMillis>(schedule_start + (periods * recurrency_period_millis));
            }

            if (profile.csChargingProfiles.chargingSchedule.chargingSchedulePeriod.empty()) {
                CHARGELAB_LOG_MESSAGE(error) << "Expected at least one chargingSchedulePeriod: " << profile;
                return std::nullopt;
            }

            // Find the associated profile
            auto const& periods = profile.csChargingProfiles.chargingSchedule.chargingSchedulePeriod;

            // Note:
            // - Returns the first period if delta is negative
            // - Returns the first period if startPeriod is non-zero and delta < startPeriod
            // - Returns the last period if all values have a startPeriod less than delta
            std::size_t active_period_index = 0;
            auto delta = timestamp - schedule_start;
            for (std::size_t i=1; i < periods.size(); i++) {
                auto const& current_period = periods[i];
                if (delta < current_period.startPeriod)
                    break;

                active_period_index = i;
            }

            return periods[active_period_index];
        }
        // persistence
        int currentJournalCapacityBytes() {
            auto const written = journal_.totalBytesWritten();
            auto const elapsed_millis = system_->steadyClockNow() - start_timestamp_;

            double accumulated_writable_bytes = journal_.storageSize();
            accumulated_writable_bytes *= kFlashLifetimeWriteCycles;
            accumulated_writable_bytes /= kRequiredFlashLifetimeMillis;
            accumulated_writable_bytes *= elapsed_millis;
            accumulated_writable_bytes += kFlashWriteCapacityOnBoot;

            auto const result = accumulated_writable_bytes - written;

            return clampToInt(result);
        }

        int clampToInt(double value) {
            if (value >= std::numeric_limits<int>::max()) {
                return std::numeric_limits<int>::max();
            } else if (value <= std::numeric_limits<int>::min()) {
                return std::numeric_limits<int>::min();
            } else {
                return static_cast<int>(value);
            }
        }

        bool allowJournalUpdate() {
            return currentJournalCapacityBytes() > 0;
        }

        std::vector<detail::JournalUpdate1_6> chargingProfilesToState() {
            std::vector<detail::JournalUpdate1_6> result;
            for (auto const& x : charging_profiles_)
                if (x.csChargingProfiles.chargingProfilePurpose != ocpp1_6::ChargingProfilePurposeType::kTxProfile)
                    result.push_back(detail::JournalUpdate1_6{x});

            return result;
        }

    private:
        std::shared_ptr<Settings> settings_;
        std::shared_ptr<SystemInterface> system_;
        std::shared_ptr<StationInterface> station_;

        // key: connectorId
        std::unordered_map<int, std::optional<SystemTimeMillis>> transaction_start_times_;
        // key: connectorId, value: transactionId
        std::unordered_map<int, int> active_transactions_;
        std::vector<ocpp1_6::SetChargingProfileReq> charging_profiles_;
        std::optional<SystemTimeMillis> next_profile_update_;

        // Saving charging profiles to storage but Tx Profiles are excluded because once power cycled the transaction would be stopped.
        chargelab::CompressedJournalJson<detail::JournalUpdate1_6> journal_;
        SteadyPointMillis start_timestamp_;
        bool charging_profile_applied_ {false};
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_POWER_MANAGEMENT_MODULE1_6_H
