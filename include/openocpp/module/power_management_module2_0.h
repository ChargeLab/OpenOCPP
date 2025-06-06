#ifndef CHARGELAB_OPEN_FIRMWARE_POWER_MANAGEMENT_MODULE2_0_H
#define CHARGELAB_OPEN_FIRMWARE_POWER_MANAGEMENT_MODULE2_0_H

#include "openocpp/module/common_templates.h"
#include "openocpp/interface/component/system_interface.h"
#include "openocpp/interface/station_interface.h"
#include "openocpp/common/settings.h"
#include "openocpp/common/macro.h"
#include "openocpp/common/compressed_journal.h"
#include "openocpp/helpers/set.h"
#include "openocpp/helpers/container.h"

namespace chargelab {
    class TransactionModule2_0;

    // TODO: We're going to have to understand when the clock shifts significantly for the purpose of re-computing
    //  active schedules. If the clock goes backwards re-compute, if the clock advances past the threshold timestamp
    //  re-compute.

    namespace detail {
        struct JournalUpdate {
            std::optional<ocpp2_0::SetChargingProfileRequest> setChargingProfileRequest = std::nullopt;
            std::optional<ocpp2_0::ClearChargingProfileRequest> clearChargingProfileRequest = std::nullopt;
            CHARGELAB_JSON_INTRUSIVE(JournalUpdate, setChargingProfileRequest, clearChargingProfileRequest)
        };

        struct ActiveSchedulePeriod {
            std::optional<ocpp2_0::ChargingSchedulePeriodType> period;
            SystemTimeMillis next_update;
        };

        struct ReportCharingProfileState {
            ocpp2_0::ChargingLimitSourceEnumType current_source = ocpp2_0::ChargingLimitSourceEnumType::kEMS;
            int current_evseId = 0;
        };
    }

    // Note: this module treats connector 0 as a separate physical connector. Profiles assigned to connector ID 0 are
    // passed as a schedule to connector 0, not to each physical connector. This is intended to allow the station to
    // define a suitable load balancing mechanism when more than one connector is present.
    class PowerManagementModule2_0 : public ServiceStateful2_0 {
        static constexpr int kMillisInDay = 1000*60*60*24;
        static constexpr int kMillisInWeek = kMillisInDay * 7;
        static constexpr int kJournalCapacityReportFrequencySeconds = 10;

        static constexpr int kFlashWriteCapacityOnBoot = 1024;
        static constexpr int kRequiredFlashLifetimeYears = 10;
        static constexpr double kRequiredFlashLifetimeMillis = kRequiredFlashLifetimeYears * (double)kMillisInDay * 365;
        static constexpr int kFlashLifetimeWriteCycles = 100000;
        friend class TransactionModule2_0;

    public:
        PowerManagementModule2_0(
                std::shared_ptr<Settings> settings,
                std::shared_ptr<SystemInterface> system,
                std::shared_ptr<StationInterface> station,
                std::unique_ptr<FlashBlockInterface> storage
        )
                : settings_(std::move(settings)),
                  system_(std::move(system)),
                  station_(std::move(station)),
                  journal_(std::move(storage), "2.0.1-1")
        {
            start_timestamp_ = system_->steadyClockNow();

            // Re-apply each of the items in the journal to recover the state
            CHARGELAB_LOG_MESSAGE(info) << "Re-applying saved charging profiles";
            journal_.visit([&](std::string_view const& text, detail::JournalUpdate const& update) {
                {
                    // Note: sleeping briefly to avoid triggering the watchdog for large journals
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(1ms);
                }

                if (update.setChargingProfileRequest.has_value()) {
                    auto const& request = update.setChargingProfileRequest.value();
                    auto response = onSetChargingProfileReqInternal(request, false);
                    if (!response.has_value()) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Persisted profile was not accepted - null response: " << text;
                    } else if (std::holds_alternative<ocpp2_0::CallError>(response.value())) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Persisted profile was not accepted - error response: " << text;
                        CHARGELAB_LOG_MESSAGE(warning) << "Error response was: " << std::get<ocpp2_0::CallError>(response.value());
                    } else if (std::holds_alternative<ocpp2_0::SetChargingProfileResponse>(response.value())) {
                        auto const& value = std::get<ocpp2_0::SetChargingProfileResponse>(response.value());
                        if (value.status != ocpp2_0::ChargingProfileStatusEnumType::kAccepted) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Persisted profile was not accepted - not accepted: " << text;
                            CHARGELAB_LOG_MESSAGE(warning) << "Response was: " << value;
                        }
                    }
                }

                if (update.clearChargingProfileRequest.has_value()) {
                    auto const& request = update.clearChargingProfileRequest.value();
                    auto response = onClearChargingProfileReqInternal(request, false);
                    if (!response.has_value()) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Persisted clear profile request was not accepted - null response: " << text;
                    } else if (std::holds_alternative<ocpp2_0::CallError>(response.value())) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Persisted clear profile request was not accepted - error response: " << text;
                        CHARGELAB_LOG_MESSAGE(warning) << "Error response was: " << std::get<ocpp2_0::CallError>(response.value());
                    } else if (std::holds_alternative<ocpp2_0::ClearChargingProfileResponse>(response.value())) {
                        auto const& value = std::get<ocpp2_0::ClearChargingProfileResponse>(response.value());
                        if (value.status != ocpp2_0::ClearChargingProfileStatusEnumType::kAccepted) {
                            CHARGELAB_LOG_MESSAGE(warning) << "Persisted clear profile request was not accepted - not accepted: " << text;
                            CHARGELAB_LOG_MESSAGE(warning) << "Response was: " << value;
                        }
                    }
                }
            });
        }

        ~PowerManagementModule2_0() override {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting PowerManagementModule2_0";
        }

    private:
        void onActiveTransactionStarted(
                int evse_id,
                std::optional<ocpp2_0::ChargingProfileType> transaction_profile,
                SteadyPointMillis start_ts
        ) {
            // As per 3.13.1. if a TxProfile is present in the RemoteStartTransaction request it overrides default
            // profile present, however it's not clear what this is intended to mean when the TxProfile expires or when
            // TxDefaultProfiles are updated within a transaction. Here we'll be adopting the conventions in OCPP 2.0.1
            // to resolve this type of ambiguity; see K.3.5. of the OCPP 2.0.1: Part 2 - Specification.
            if (transaction_profile.has_value()) {
                charging_profiles_.resize(charging_profiles_.size()+1);
                charging_profiles_.back() = ocpp2_0::SetChargingProfileRequest {
                    evse_id,
                    transaction_profile.value()
                };
                last_charging_profile_update_ = std::nullopt; // Re-calculate the charging schedule
            }

            transaction_start_times_[evse_id] = start_ts;
        }

        void onActiveTransactionIdAssigned(int evse_id, std::string const& transaction_id) {
            // Update transaction IDs on TxProfiles
            for (auto& profile : charging_profiles_) {
                if (profile.chargingProfile.chargingProfilePurpose != ocpp2_0::ChargingProfilePurposeEnumType::kTxProfile)
                    continue;

                // TODO: Can TxProfiles be assigned to connector 0, or is that prohibited?
                if (profile.evseId != evse_id)
                    continue;

                profile.chargingProfile.transactionId = transaction_id;
            }

            transaction_id_map_[evse_id] = transaction_id;
        }

        void onActiveTransactionFinished(int evse_id) {
            charging_profiles_.erase(
                    std::remove_if(
                            charging_profiles_.begin(),
                            charging_profiles_.end(),
                            [&](ocpp2_0::SetChargingProfileRequest const& profile) {
                                if (profile.chargingProfile.chargingProfilePurpose != ocpp2_0::ChargingProfilePurposeEnumType::kTxProfile)
                                    return false;
                                if (profile.evseId != evse_id)
                                    return false;

                                return true;
                            }
                    ),
                    charging_profiles_.end()
            );

            transaction_start_times_[evse_id] = std::nullopt;
            transaction_id_map_[evse_id] = std::nullopt;
        }

        std::optional<ocpp2_0::ResponseToRequest <ocpp2_0::SetChargingProfileResponse>>
        onSetChargingProfileReq(const ocpp2_0::SetChargingProfileRequest &req) override {
            return onSetChargingProfileReqInternal(req, true);
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::ClearChargingProfileResponse>>
        onClearChargingProfileReq(const ocpp2_0::ClearChargingProfileRequest& req) override {
            return onClearChargingProfileReqInternal(req, true);
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetChargingProfilesResponse>>
        onGetChargingProfilesReq(const ocpp2_0::GetChargingProfilesRequest &request) override {
            if (pending_get_charging_profiles_request_.has_value()) {
                return ocpp2_0::CallError {
                        ocpp2_0::ErrorCode::kInternalError,
                        "Another GetChargingProfilesRequest operation was already running",
                        common::RawJson::empty_object()
                };
            }

            CHARGELAB_LOG_MESSAGE(info) << "Active charging profiles: " << charging_profiles_;

            // Note: no explicit requirement found in K09
            detail::ReportCharingProfileState state {};
            if (totalMatchingProfiles(request, state) <= 0 && totalRemainingProfiles(request, state) <= 0) {
                return ocpp2_0::GetChargingProfilesResponse {ocpp2_0::GetChargingProfileStatusEnumType::kNoProfiles};
            }

            pending_get_charging_profiles_request_ = std::make_pair(request, state);
            return ocpp2_0::GetChargingProfilesResponse {ocpp2_0::GetChargingProfileStatusEnumType::kAccepted};
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetCompositeScheduleResponse>>
        onGetCompositeScheduleReq(const ocpp2_0::GetCompositeScheduleRequest& req) override {
            (void)req;
            return std::nullopt;
        }

        void runStep(ocpp2_0::OcppRemote& remote) override {
            reportJournalCapacity();

            if (pending_get_charging_profiles_request_.has_value())
                sendChargingProfilesReport(remote);

            auto const now = system_->systemClockNow();
            if (last_charging_profile_update_.has_value()) {
                if (now - last_charging_profile_update_->first < 0) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Re-computing charging profiles: system clock decreased";
                } else if (now - last_charging_profile_update_->second >= 0) {
                    CHARGELAB_LOG_MESSAGE(info) << "Re-computing charging profiles: passed re-compute threshold";
                } else {
                    return;
                }
            }

            last_charging_profile_update_ = std::make_pair(now, updateActiveChargingProfiles(now));

            // TODO: Charging profiles shouldn't be applied until the clock is synchronized
            // TODO: Support for offline overrides should be preserved for now?
        }

    private:
        std::optional<ocpp2_0::ResponseToRequest <ocpp2_0::SetChargingProfileResponse>>
        onSetChargingProfileReqInternal(const ocpp2_0::SetChargingProfileRequest &req, bool persist_to_journal) {
            // K01.FR.06
            if (req.chargingProfile.chargingProfilePurpose != ocpp2_0::ChargingProfilePurposeEnumType::kTxProfile) {
                for (auto const& x : charging_profiles_) {
                    if (x.chargingProfile.stackLevel != req.chargingProfile.stackLevel)
                        continue;
                    if (x.chargingProfile.chargingProfilePurpose != req.chargingProfile.chargingProfilePurpose)
                        continue;
                    if (x.evseId != req.evseId)
                        continue;
                    if (x.chargingProfile.id == req.chargingProfile.id)
                        continue;

                    auto const ts_min = (SystemTimeMillis)std::numeric_limits<std::int64_t>::min();
                    auto const ts_max = (SystemTimeMillis)std::numeric_limits<std::int64_t>::max();
                    SystemTimeMillis req_from = ts_min, req_to = ts_max;
                    if (req.chargingProfile.validFrom.has_value())
                        req_from = optional::GetOrDefault(req.chargingProfile.validFrom->getTimestamp(), req_from);
                    if (req.chargingProfile.validTo.has_value())
                        req_to = optional::GetOrDefault(req.chargingProfile.validTo->getTimestamp(), req_to);

                    SystemTimeMillis x_from = ts_min, x_to = ts_max;
                    if (x.chargingProfile.validFrom.has_value())
                        x_from = optional::GetOrDefault(x.chargingProfile.validFrom->getTimestamp(), x_from);
                    if (x.chargingProfile.validTo.has_value())
                        x_to = optional::GetOrDefault(x.chargingProfile.validTo->getTimestamp(), x_to);

                    // Allow non-overlapping
                    if (!(req_from < x_to && x_from < req_to))
                        continue;

                    return ocpp2_0::CallError {
                            // TODO: Is this a reasonable choice?
                            ocpp2_0::ErrorCode::kProtocolError,
                            "Request violates K01.FR.06",
                            common::RawJson::empty_object()
                    };
                }
            }

            // K01.FR.09
            if (req.chargingProfile.chargingProfilePurpose == ocpp2_0::ChargingProfilePurposeEnumType::kTxProfile) {
                if (!transaction_start_times_[req.evseId].has_value())
                    return ocpp2_0::SetChargingProfileResponse{ocpp2_0::ChargingProfileStatusEnumType::kRejected};
            }

            // K01.FR.16
            if (req.chargingProfile.chargingProfilePurpose == ocpp2_0::ChargingProfilePurposeEnumType::kTxProfile) {
                if (req.evseId <= 0) {
                    return ocpp2_0::CallError {
                            // TODO: Is this a reasonable choice?
                            ocpp2_0::ErrorCode::kProtocolError,
                            "Request violates K01.FR.16",
                            common::RawJson::empty_object()
                    };
                }
            }

            // K01.FR.19
            for (auto const& schedule : req.chargingProfile.chargingSchedule) {
                for (auto const& period : schedule.chargingSchedulePeriod) {
                    if (period.phaseToUse.has_value() && period.numberPhases != std::make_optional(1)) {
                        return ocpp2_0::CallError {
                                // TODO: Is this a reasonable choice?
                                ocpp2_0::ErrorCode::kProtocolError,
                                "Request violates K01.FR.19",
                                common::RawJson::empty_object()
                        };
                    }
                }
            }

            // K01.FR.22
            if (req.chargingProfile.chargingProfilePurpose == ocpp2_0::ChargingProfilePurposeEnumType::kChargingStationExternalConstraints) {
                return ocpp2_0::CallError {
                        // TODO: Is this a reasonable choice?
                        ocpp2_0::ErrorCode::kProtocolError,
                        "Request violates K01.FR.22",
                        common::RawJson::empty_object()
                };
            }

            // K01.FR.26
            {
                auto value_as_enum = ocpp2_0::ChargingRateUnitEnumType::from_string(settings_->ChargingScheduleChargingRateUnit.getValue());
                for (auto const &schedule: req.chargingProfile.chargingSchedule) {
                    if (schedule.chargingRateUnit != value_as_enum)
                        return ocpp2_0::SetChargingProfileResponse {ocpp2_0::ChargingProfileStatusEnumType::kRejected};
                }
            }

            // K01.FR.28
            if (req.evseId != 0 && !set::contains(getEvseIds(), req.evseId))
                return ocpp2_0::SetChargingProfileResponse {ocpp2_0::ChargingProfileStatusEnumType::kRejected};

            // Note: ISO-15118 is *NOT* supported at the moment.
            // K01.FR.30 - Each non ISO-15118 profile should come with exactly one charging schedule
            if (req.chargingProfile.chargingSchedule.size() != 1) {
                return ocpp2_0::CallError {
                        // TODO: Is this a reasonable choice?
                        ocpp2_0::ErrorCode::kProtocolError,
                        "ISO 15118 not support and/or request violates K01.FR.30",
                        common::RawJson::empty_object()
                };
            }

            for (auto const& schedule : req.chargingProfile.chargingSchedule) {
                if (schedule.chargingSchedulePeriod.empty())
                    continue;

                // K01.FR.31
                if (schedule.chargingSchedulePeriod.front().startPeriod != 0) {
                    return ocpp2_0::CallError {
                            // TODO: Is this a reasonable choice?
                            ocpp2_0::ErrorCode::kProtocolError,
                            "Request violates K01.FR.19",
                            common::RawJson::empty_object()
                    };
                }

                // K01.FR.35
                for (std::size_t i=1; i < schedule.chargingSchedulePeriod.size(); i++) {
                    if (schedule.chargingSchedulePeriod[i-1].startPeriod > schedule.chargingSchedulePeriod[i].startPeriod) {
                        return ocpp2_0::CallError {
                                // TODO: Is this a reasonable choice?
                                ocpp2_0::ErrorCode::kProtocolError,
                                "Request violates K01.FR.35",
                                common::RawJson::empty_object()
                        };
                    }
                }
            }

            if (req.chargingProfile.chargingProfilePurpose == ocpp2_0::ChargingProfilePurposeEnumType::kTxProfile) {
                // K01.FR.03
                if (!req.chargingProfile.transactionId.has_value()) {
                    return ocpp2_0::CallError {
                            // TODO: Is this a reasonable choice?
                            ocpp2_0::ErrorCode::kProtocolError,
                            "Request violates K01.FR.03",
                            common::RawJson::empty_object()
                    };
                }

                // K01.FR.33
                if (req.chargingProfile.transactionId != transaction_id_map_[req.evseId])
                    return ocpp2_0::SetChargingProfileResponse {ocpp2_0::ChargingProfileStatusEnumType::kRejected};
            }

            // K01.FR.38
            if (req.chargingProfile.chargingProfilePurpose == ocpp2_0::ChargingProfilePurposeEnumType::kChargingStationMaxProfile) {
                if (req.chargingProfile.chargingProfileKind == ocpp2_0::ChargingProfileKindEnumType::kRecurring) {
                    return ocpp2_0::CallError {
                            // TODO: Is this a reasonable choice?
                            ocpp2_0::ErrorCode::kProtocolError,
                            "Request violates K01.FR.38",
                            common::RawJson::empty_object()
                    };
                }
            }

            // K01.FR.39
            if (req.chargingProfile.chargingProfilePurpose == ocpp2_0::ChargingProfilePurposeEnumType::kTxProfile) {
                for (auto const& x : charging_profiles_) {
                    if (x.chargingProfile.stackLevel != req.chargingProfile.stackLevel)
                        continue;
                    if (x.chargingProfile.transactionId != req.chargingProfile.transactionId)
                        continue;
                    if (x.chargingProfile.chargingProfilePurpose != ocpp2_0::ChargingProfilePurposeEnumType::kTxProfile)
                        continue;
                    if (x.chargingProfile.id == req.chargingProfile.id)
                        continue;

                    return ocpp2_0::CallError {
                            // TODO: Is this a reasonable choice?
                            ocpp2_0::ErrorCode::kProtocolError,
                            "Request violates K01.FR.39",
                            common::RawJson::empty_object()
                    };
                }
            }

            // K01.FR.40
            {
                auto const kind = req.chargingProfile.chargingProfileKind;
                using kind_type = ocpp2_0::ChargingProfileKindEnumType;
                if (kind == kind_type::kAbsolute || kind == kind_type::kRecurring) {
                    for (auto const& schedule : req.chargingProfile.chargingSchedule) {
                        if (!schedule.startSchedule.has_value()) {
                            return ocpp2_0::CallError {
                                    // TODO: Is this a reasonable choice?
                                    ocpp2_0::ErrorCode::kProtocolError,
                                    "Request violates K01.FR.40",
                                    common::RawJson::empty_object()
                            };
                        }
                    }
                }
            }

            // K01.FR.41
            if (req.chargingProfile.chargingProfileKind == ocpp2_0::ChargingProfileKindEnumType::kRelative) {
                for (auto const& schedule : req.chargingProfile.chargingSchedule) {
                    if (schedule.startSchedule.has_value()) {
                        return ocpp2_0::CallError {
                                // TODO: Is this a reasonable choice?
                                ocpp2_0::ErrorCode::kProtocolError,
                                "Request violates K01.FR.41",
                                common::RawJson::empty_object()
                        };
                    }
                }
            }

            // K01.FR.52
            if (req.chargingProfile.chargingProfilePurpose == ocpp2_0::ChargingProfilePurposeEnumType::kTxDefaultProfile && req.evseId == 0) {
                for (auto const& x : charging_profiles_) {
                    if (x.chargingProfile.chargingProfilePurpose != req.chargingProfile.chargingProfilePurpose)
                        continue;
                    if (x.chargingProfile.stackLevel != req.chargingProfile.stackLevel)
                        continue;
                    if (x.evseId == 0)
                        continue;
                    if (x.chargingProfile.id == req.chargingProfile.id)
                        continue;

                    return ocpp2_0::SetChargingProfileResponse {
                            ocpp2_0::ChargingProfileStatusEnumType::kRejected,
                            ocpp2_0::StatusInfoType {{"DuplicateProfile"}}
                    };
                }
            }

            // K01.FR.53
            if (req.chargingProfile.chargingProfilePurpose == ocpp2_0::ChargingProfilePurposeEnumType::kTxDefaultProfile && req.evseId > 0) {
                for (auto const& x : charging_profiles_) {
                    if (x.chargingProfile.chargingProfilePurpose != req.chargingProfile.chargingProfilePurpose)
                        continue;
                    if (x.chargingProfile.stackLevel != req.chargingProfile.stackLevel)
                        continue;
                    if (x.evseId != 0)
                        continue;
                    if (x.chargingProfile.id == req.chargingProfile.id)
                        continue;

                    return ocpp2_0::SetChargingProfileResponse {
                            ocpp2_0::ChargingProfileStatusEnumType::kRejected,
                            ocpp2_0::StatusInfoType {{"DuplicateProfile"}}
                    };
                }
            }

            // TODO: does the 2.0.1 spec provide any guidance on how requests should be rejected when they would violate
            //       technical station limits (in this case: when the station needs to limit writes to flash and cannot
            //       accept the charging profile update)?

            // If a profile with the same ID exists replace it, otherwise add a new profile
            // K01.FR.05 - note that the charging profile is never of type ChargingStationExternalConstraints here; see
            //             K01.FR.22 above.
            bool replaced_existing = false;
            for (auto& x : charging_profiles_) {
                if (x.chargingProfile.id != req.chargingProfile.id)
                    continue;
                if (x == req) {
                    // If the charging profile hasn't changed the request can be ignored
                    CHARGELAB_LOG_MESSAGE(info) << "Charging profile hasn't changed - ignoring";
                    return ocpp2_0::SetChargingProfileResponse {ocpp2_0::ChargingProfileStatusEnumType::kAccepted};
                }

                if (persist_to_journal) {
                    if (!allowJournalUpdate()) {
                        return ocpp2_0::SetChargingProfileResponse{
                                ocpp2_0::ChargingProfileStatusEnumType::kRejected,
                                ocpp2_0::StatusInfoType{
                                        "RateLimitExceeded",
                                        "Maximum charging profile update rate exceeded - please try again later"
                                }
                        };
                    } else if (!journal_.addUpdate(chargingProfilesToState(), detail::JournalUpdate{req})) {
                        return ocpp2_0::SetChargingProfileResponse{
                                ocpp2_0::ChargingProfileStatusEnumType::kRejected,
                                ocpp2_0::StatusInfoType{
                                        "FailedPersisting",
                                        "Failed writing update to flash storage"
                                }
                        };
                    }
                }

                x = req;
                replaced_existing = true;
                break;
            }
            if (!replaced_existing) {
                if (persist_to_journal) {
                    if (!allowJournalUpdate()) {
                        return ocpp2_0::SetChargingProfileResponse{
                                ocpp2_0::ChargingProfileStatusEnumType::kRejected,
                                ocpp2_0::StatusInfoType{
                                        "RateLimitExceeded",
                                        "Maximum charging profile update rate exceeded - please try again later"
                                }
                        };
                    } else if (!journal_.addUpdate(chargingProfilesToState(), detail::JournalUpdate{req})) {
                        return ocpp2_0::SetChargingProfileResponse{
                                ocpp2_0::ChargingProfileStatusEnumType::kRejected,
                                ocpp2_0::StatusInfoType{
                                        "FailedPersisting",
                                        "Failed writing update to flash storage"
                                }
                        };
                    }
                }

                // Note: avoiding push_back to limit heap usage to active elements.
                charging_profiles_.resize(charging_profiles_.size()+1);
                charging_profiles_.back() = req;
            }

            last_charging_profile_update_ = std::nullopt;
            return ocpp2_0::SetChargingProfileResponse {ocpp2_0::ChargingProfileStatusEnumType::kAccepted};
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::ClearChargingProfileResponse>>
        onClearChargingProfileReqInternal(const ocpp2_0::ClearChargingProfileRequest& req, bool persist_to_journal) {
            // K10.FR.02
            if (!req.chargingProfileId.has_value()) {
                bool criteria_set = false;
                if (req.chargingProfileCriteria.has_value()) {
                    auto const& x = req.chargingProfileCriteria.value();
                    criteria_set = x.evseId.has_value() || x.stackLevel.has_value() || x.chargingProfilePurpose.has_value();
                }

                if (!criteria_set) {
                    return ocpp2_0::CallError {
                            // TODO: Is this a reasonable choice?
                            ocpp2_0::ErrorCode::kProtocolError,
                            "Request violates K10.FR.02",
                            common::RawJson::empty_object()
                    };
                }
            }

            // K10.FR.06
            if (req.chargingProfileCriteria.has_value()) {
                if (req.chargingProfileCriteria->chargingProfilePurpose == std::make_optional(ocpp2_0::ChargingProfilePurposeEnumType::kChargingStationExternalConstraints)) {
                    return ocpp2_0::CallError {
                            // TODO: Is this a reasonable choice?
                            ocpp2_0::ErrorCode::kProtocolError,
                            "Request violates K10.FR.06",
                            common::RawJson::empty_object()
                    };
                }
            }

            auto const new_end = std::remove_if(
                    charging_profiles_.begin(),
                    charging_profiles_.end(),
                    [&](ocpp2_0::SetChargingProfileRequest const& x) {
                        // K10.FR.03 and K10.FR.04
                        if (x.chargingProfile.chargingProfilePurpose == ocpp2_0::ChargingProfilePurposeEnumType::kChargingStationExternalConstraints)
                            return false;

                        // K10.FR.03
                        if (req.chargingProfileId == std::make_optional(x.chargingProfile.id))
                            return true;

                        // K10.FR.04
                        if (req.chargingProfileCriteria.has_value()) {
                            auto const& y = req.chargingProfileCriteria.value();
                            if (y.chargingProfilePurpose == std::make_optional(x.chargingProfile.chargingProfilePurpose))
                                return true;
                            if (y.stackLevel == std::make_optional(x.chargingProfile.stackLevel))
                                return true;
                            if (y.evseId == std::make_optional(x.evseId))
                                return true;
                        }

                        return false;
                    }
            );

            // K10.FR.01, K10.FR.08, and K10.FR.09
            auto const matching_profiles = std::distance(new_end, charging_profiles_.end());
            if (matching_profiles == 0)
                return ocpp2_0::ClearChargingProfileResponse {ocpp2_0::ClearChargingProfileStatusEnumType::kUnknown};

            if (persist_to_journal) {
                if (!allowJournalUpdate()) {
                    return ocpp2_0::CallError{
                            ocpp2_0::ErrorCode::kInternalError,
                            "Charging profile update rate limit exceeded - please try again later",
                            common::RawJson::empty_object()
                    };
                } else if (!journal_.addUpdate(chargingProfilesToState(), detail::JournalUpdate{std::nullopt, req})) {
                    return ocpp2_0::CallError{
                            ocpp2_0::ErrorCode::kInternalError,
                            "Failed writing update to flash storage",
                            common::RawJson::empty_object()
                    };
                }
            }

            charging_profiles_.erase(new_end, charging_profiles_.end());
            last_charging_profile_update_ = std::nullopt;
            return ocpp2_0::ClearChargingProfileResponse {ocpp2_0::ClearChargingProfileStatusEnumType::kAccepted};
        }

        void reportJournalCapacity() {
            auto const now = system_->steadyClockNow();
            if (last_reported_journal_capacity_.has_value()) {
                auto const delta = now - last_reported_journal_capacity_.value();
                if (delta / 1000 < kJournalCapacityReportFrequencySeconds)
                    return;
            }

            last_reported_journal_capacity_ = now;
            CHARGELAB_LOG_MESSAGE(info) << "Current journal capacity: " << currentJournalCapacityBytes() << " bytes";
        }

        void sendChargingProfilesReport(ocpp2_0::OcppRemote& remote) {
            bool finished = false;

            {
                auto const &req = pending_get_charging_profiles_request_->first;
                auto &state = pending_get_charging_profiles_request_->second;
                while (totalMatchingProfiles(req, state) <= 0) {
                    if (!advanceState(state)) {
                        finished = true;
                        break;
                    }
                }

                if (!finished) {
                    auto const call = ocpp2_0::ReportChargingProfilesRequest{
                            req.requestId,
                            state.current_source,
                            totalRemainingProfiles(req, state) > 0,
                            state.current_evseId,
                            {[&](std::function<void(ocpp2_0::ChargingProfileType const &)> const &visitor) {
                                for (auto const &x: charging_profiles_) {
                                    if (includeProfile(req, state, x))
                                        visitor(x.chargingProfile);
                                }
                            }}
                    };

                    // Note: intentionally *not* attempting to retry report requests if they're rejected or the server
                    // fails to respond.
                    if (remote.sendCall(call).has_value()) {
                        if (!advanceState(state))
                            finished = true;
                    }
                }
            }

            if (finished) {
                pending_get_charging_profiles_request_ = std::nullopt;
            }
        }

        SystemTimeMillis updateActiveChargingProfiles(SystemTimeMillis now) {
            sortChargingProfiles();
            auto next_update = static_cast<SystemTimeMillis>(now + 60*1000);

            station_->setActiveChargePointMaxProfiles(getActiveChargingProfiles(0, now, next_update));
            for (auto const& evse_id : getEvseIds())
                station_->setActiveEvseProfiles(evse_id, getActiveChargingProfiles(evse_id, now, next_update));

            return next_update;
        }

        std::vector<StationInterface::schedule_type2_0> getActiveChargingProfiles(
                int evse_id,
                SystemTimeMillis now,
                SystemTimeMillis& next_update
        ) {
            // Calculate active profiles according to the rules in K. Smart Charging - 3. Charging Profiles
            // Note: it seems reasonable that the intent of the rules defined in 3.5 is to determine how active profiles
            //       are selected *for a particular EVSE ID*, not across a charging station. That is, for each EVSE ID
            //       the stacking rules are applied to determine which profiles are active.

            std::map<ocpp2_0::ChargingProfilePurposeEnumType, StationInterface::schedule_type2_0> active_profiles;
            for (auto const& charging_profile : charging_profiles_) {
                if (evse_id == 0) {
                    // If the EVSE ID is 0 then only the ChargingStationMaxProfiles apply
                    if (charging_profile.chargingProfile.chargingProfilePurpose != ocpp2_0::ChargingProfilePurposeEnumType::kChargingStationMaxProfile)
                        continue;
                } else {
                    // If the EVSE ID is not 0 then everything *other* than the ChargingStationMaxProfiles apply
                    if (charging_profile.chargingProfile.chargingProfilePurpose == ocpp2_0::ChargingProfilePurposeEnumType::kChargingStationMaxProfile)
                        continue;
                }

                if (charging_profile.evseId != 0 && charging_profile.evseId != evse_id)
                    continue;

                std::optional<SystemTimeMillis> start_time;
                if (transaction_start_times_[evse_id].has_value()) {
                    auto const elapsed = system_->steadyClockNow() - transaction_start_times_[evse_id].value();
                    auto const start_time_ts = static_cast<SystemTimeMillis>(system_->systemClockNow() - elapsed);
                    start_time = std::make_optional(start_time_ts);
                }

                // Note: only charging schedules with a defined period for the current time are considered here.
                // Quote: [...] that has a schedule period defined for that time [...]
                auto active_period = getActiveSchedulePeriod(charging_profile, now, start_time);
                next_update = std::min(next_update, active_period.next_update);

                if (!active_period.period.has_value())
                    continue;

                if (charging_profile.chargingProfile.validFrom.has_value()) {
                    auto const ts = charging_profile.chargingProfile.validFrom->getTimestamp();
                    if (ts.has_value() && ts.value() > now)
                        break;
                }
                if (charging_profile.chargingProfile.validTo.has_value()) {
                    auto const ts = charging_profile.chargingProfile.validTo->getTimestamp();
                    if (ts.has_value() && ts.value() <= now)
                        break;
                }

                auto it = active_profiles.find(charging_profile.chargingProfile.chargingProfilePurpose);
                if (it == active_profiles.end()) {
                    active_profiles.insert(std::make_pair(
                            charging_profile.chargingProfile.chargingProfilePurpose,
                            StationInterface::schedule_type2_0 {charging_profile, active_period.period.value()}
                    ));
                } else {
                    // Quote: [...] a ChargingProfile with a higher stack level overrules a ChargingSchedule from a
                    //        ChargingProfile with a lower stack level.
                    if (charging_profile.chargingProfile.stackLevel > it->second.first.chargingProfile.stackLevel)
                        it->second = StationInterface::schedule_type2_0 {charging_profile, active_period.period.value()};
                }
            }

            // Quote: The only exception is when both a TxDefaultProfile and a TxProfile are valid. In that case, the
            //        TxProfile will always overrule the TxDefaultProfile, hence the Composite Schedule will not take
            //        the leading profile of purpose TxDefaultProfile into account in this specific situation.
            if (container::contains(active_profiles, ocpp2_0::ChargingProfilePurposeEnumType::kTxProfile)) {
                auto it = active_profiles.find(ocpp2_0::ChargingProfilePurposeEnumType::kTxDefaultProfile);
                if (it != active_profiles.end())
                    active_profiles.erase(it);
            }

            std::vector<StationInterface::schedule_type2_0> result;
            for (auto const& x : active_profiles)
                result.push_back(x.second);

            return result;
        }

        detail::ActiveSchedulePeriod getActiveSchedulePeriod(
                ocpp2_0::SetChargingProfileRequest const& profile,
                SystemTimeMillis const now,
                std::optional<SystemTimeMillis> const& tx_power_path_closed_ts
        ) {
            // Note: ISO-15118 is *NOT* supported at the moment.
            // K01.FR.30 - Each non ISO-15118 profile should come with exactly one charging schedule
            auto const max_ts = static_cast <SystemTimeMillis> (std::numeric_limits<int64_t>::max());
            if (profile.chargingProfile.chargingSchedule.size() != 1) {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad charging profile - charging schedule size was not 1";
                return {std::nullopt, max_ts};
            }

            auto const& schedule = profile.chargingProfile.chargingSchedule.front();

            std::optional<SystemTimeMillis> start_schedule;
            if (schedule.startSchedule.has_value())
                start_schedule = schedule.startSchedule->getTimestamp();

            // K01.FR.30
            if (start_schedule.has_value() && start_schedule.value() > now)
                return {std::nullopt, start_schedule.value()};

            if (profile.chargingProfile.chargingProfileKind == ocpp2_0::ChargingProfileKindEnumType::kAbsolute) {
                // Quote: Schedule periods are relative to a fixed point in time defined in the schedule. This requires that startSchedule
                //        is set to a starting point in time.
                if (!start_schedule.has_value()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Bad charging profile - missing expected startSchedule";
                    return {std::nullopt, max_ts};
                }

                auto const start_ts = start_schedule.value();
                auto next_update = max_ts;
                if (schedule.duration.has_value()) {
                    // Schedule has expired
                    if (now >= start_ts + schedule.duration.value()*1000)
                        return {std::nullopt, max_ts};

                    next_update = static_cast<SystemTimeMillis> (start_ts + schedule.duration.value()*1000);
                }

                if (schedule.chargingSchedulePeriod.empty()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Bad charging profile - expected at least one chargingSchedulePeriod";
                    return {std::nullopt, max_ts};
                }

                auto const elapsed_seconds = (now - start_ts)/1000;
                std::size_t index;
                for (index=1; index < schedule.chargingSchedulePeriod.size(); index++) {
                    auto const start_period = schedule.chargingSchedulePeriod[index].startPeriod;
                    if (start_period > elapsed_seconds) {
                        next_update = std::min(next_update, static_cast<SystemTimeMillis> (start_ts + start_period*1000));
                        break;
                    }
                }

                return {schedule.chargingSchedulePeriod[index-1], next_update};
            } else if (profile.chargingProfile.chargingProfileKind == ocpp2_0::ChargingProfileKindEnumType::kRecurring) {
                // Quote: The schedule restarts periodically at the first schedule period. To be most useful, this requires that
                //        startSchedule is set to a starting point in time.
                if (!start_schedule.has_value()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Bad charging profile - missing expected startSchedule";
                    return {std::nullopt, max_ts};
                }

                if (!profile.chargingProfile.recurrencyKind.has_value()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Bad charging profile - missing expected recurrencyKind for Recurring charging profile";
                    return {std::nullopt, max_ts};
                }

                auto next_update = max_ts;
                auto start_ts = start_schedule.value();
                auto elapsed = now - start_ts;
                switch (profile.chargingProfile.recurrencyKind.value()) {
                    case ocpp2_0::RecurrencyKindEnumType::kDaily:
                        start_ts = static_cast<SystemTimeMillis> (start_ts + (elapsed/kMillisInDay)*kMillisInDay);
                        elapsed = now - start_ts;
                        next_update = static_cast<SystemTimeMillis> (start_ts + kMillisInDay);
                        break;

                    case ocpp2_0::RecurrencyKindEnumType::kWeekly:
                        start_ts = static_cast<SystemTimeMillis> (start_ts + (elapsed/kMillisInWeek)*kMillisInWeek);
                        elapsed = now - start_ts;
                        next_update = static_cast<SystemTimeMillis> (start_ts + kMillisInWeek);
                        break;

                    case ocpp2_0::RecurrencyKindEnumType::kValueNotFoundInEnum:
                        CHARGELAB_LOG_MESSAGE(warning) << "Bad charging profile - invalid recurrencyKind";
                        return {std::nullopt, max_ts};
                }

                if (schedule.duration.has_value()) {
                    // Schedule has expired
                    if (now >= start_ts + schedule.duration.value()*1000)
                        return {std::nullopt, next_update};

                    next_update = std::min(next_update, static_cast<SystemTimeMillis> (start_ts + schedule.duration.value()*1000));
                }

                if (schedule.chargingSchedulePeriod.empty()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Bad charging profile - expected at least one chargingSchedulePeriod";
                    return {std::nullopt, max_ts};
                }

                auto const elapsed_seconds = (now - start_ts)/1000;
                std::size_t index;
                for (index=1; index < schedule.chargingSchedulePeriod.size(); index++) {
                    auto const start_period = schedule.chargingSchedulePeriod[index].startPeriod;
                    if (start_period > elapsed_seconds) {
                        next_update = std::min(next_update, static_cast<SystemTimeMillis> (start_ts + start_period*1000));
                        break;
                    }
                }

                return {schedule.chargingSchedulePeriod[index-1], next_update};
            } else if (profile.chargingProfile.chargingProfileKind == ocpp2_0::ChargingProfileKindEnumType::kRelative) {
                // Quote: Charging schedule periods should start when the EVSE is ready to deliver energy. i.e. when the
                //        EV driver is authorized and the EV is connected. When a ChargingProfile is received for a
                //        transaction that is already charging, then the charging schedule periods should remain
                //        relative to the PowerPathClosed moment. No value for startSchedule should be supplied.

                // If a transaction is not yet running interpret the charging profile as applying now
                auto start_ts = now;
                if (tx_power_path_closed_ts.has_value())
                    start_ts = tx_power_path_closed_ts.value();

                auto next_update = max_ts;
                if (schedule.duration.has_value()) {
                    // Schedule has expired
                    if (now >= start_ts + schedule.duration.value()*1000)
                        return {std::nullopt, max_ts};

                    next_update = static_cast<SystemTimeMillis> (start_ts + schedule.duration.value()*1000);
                }

                if (schedule.chargingSchedulePeriod.empty()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Bad charging profile - expected at least one chargingSchedulePeriod";
                    return {std::nullopt, max_ts};
                }

                auto const elapsed_seconds = (now - start_ts)/1000;
                std::size_t index;
                for (index=1; index < schedule.chargingSchedulePeriod.size(); index++) {
                    auto const start_period = schedule.chargingSchedulePeriod[index].startPeriod;
                    if (start_period > elapsed_seconds) {
                        next_update = std::min(next_update, static_cast<SystemTimeMillis> (start_ts + start_period*1000));
                        break;
                    }
                }

                return {schedule.chargingSchedulePeriod[index-1], next_update};
            } else {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad charging profile - invalid chargingProfileKind";
                return {std::nullopt, max_ts};
            }
        }

        void sortChargingProfiles() {
            std::sort(
                    charging_profiles_.begin(),
                    charging_profiles_.end(),
                    [](ocpp2_0::SetChargingProfileRequest const& lhs, ocpp2_0::SetChargingProfileRequest const& rhs) {
                        // lhs < rhs
                        if (!lhs.chargingProfile.validFrom.has_value()) {
                            return rhs.chargingProfile.validFrom.has_value();
                        } else if (!rhs.chargingProfile.validFrom.has_value()) {
                            // NON-EMPTY < EMPTY
                            return false;
                        } else {
                            auto const& lhs_ts = lhs.chargingProfile.validFrom->getTimestamp();
                            auto const& rhs_ts = rhs.chargingProfile.validFrom->getTimestamp();
                            return lhs_ts < rhs_ts;
                        }
                    }
            );
        }

        std::vector<detail::JournalUpdate> chargingProfilesToState() {
            std::vector<detail::JournalUpdate> result;
            for (auto const& x : charging_profiles_)
                result.push_back(detail::JournalUpdate{x});

            return result;
        }

        int currentJournalCapacityBytes() {
            auto const written = journal_.totalBytesWritten();
            auto const elapsed_millis = system_->steadyClockNow() - start_timestamp_;

            double accumulator = journal_.storageSize();
            accumulator *= kFlashLifetimeWriteCycles;
            accumulator /= kRequiredFlashLifetimeMillis;
            accumulator *= elapsed_millis;
            accumulator += kFlashWriteCapacityOnBoot;

            auto const result = accumulator - written;
            if (result >= std::numeric_limits<int>::max()) {
                return std::numeric_limits<int>::max();
            } else if (result <= std::numeric_limits<int>::min()) {
                return std::numeric_limits<int>::min();
            } else {
                return static_cast<int>(result);
            }
        }

        bool allowJournalUpdate() {
            return currentJournalCapacityBytes() > 0;
        }

        bool includeProfile(
                ocpp2_0::GetChargingProfilesRequest const& request,
                detail::ReportCharingProfileState const& state,
                ocpp2_0::SetChargingProfileRequest const& profile
        ) {
            //CHARGELAB_LOG_MESSAGE(info) << "Checking for match: [" << state.current_source << "," << state.current_evseId << "]: " << profile;

            // Note: only CSO profiles are currently supported
            auto const profile_source = ocpp2_0::ChargingLimitSourceEnumType::kCSO;

            if (state.current_source != profile_source)
                return false;
            if (state.current_evseId != profile.evseId)
                return false;
            if (request.evseId.has_value() && request.evseId.value() != profile.evseId)
                return false;

            if (request.chargingProfile.chargingLimitSource.has_value()) {
                bool found = false;
                for (auto const& x : request.chargingProfile.chargingLimitSource.value()) {
                    if (x == profile_source) {
                        found = true;
                        break;
                    }
                }

                if (!found)
                    return false;
            }

            {
                auto const& purpose = request.chargingProfile.chargingProfilePurpose;
                if (purpose.has_value() && purpose.value() != profile.chargingProfile.chargingProfilePurpose)
                    return false;
            }

            {
                auto const& stackLevel = request.chargingProfile.stackLevel;
                if (stackLevel.has_value() && stackLevel.value() != profile.chargingProfile.stackLevel)
                    return false;
            }

            {
                auto const& chargingProfileIds = request.chargingProfile.chargingProfileId;
                if (chargingProfileIds.has_value()) {
                    bool found = false;
                    for (auto const& x : chargingProfileIds.value()) {
                        if (profile.chargingProfile.id == x) {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                        return false;
                }
            }

            return true;
        }

        int totalMatchingProfiles(
                ocpp2_0::GetChargingProfilesRequest const& request,
                detail::ReportCharingProfileState const& state
        ) {
            int result = 0;
            for (auto const& x : charging_profiles_)
                if (includeProfile(request, state, x))
                    result++;

            //CHARGELAB_LOG_MESSAGE(info) << "Total matching profiles for [" << state.current_source << "," << state.current_evseId << "]: " << result;
            return result;
        }

        bool advanceState(detail::ReportCharingProfileState& state) {
            std::optional<int> next_evseId;
            for (auto const& x : getEvseIds()) {
                if (x <= state.current_evseId)
                    continue;
                if (next_evseId.has_value()) {
                    next_evseId = std::min(next_evseId.value(), x);
                } else {
                    next_evseId = x;
                }
            }

            if (next_evseId.has_value()) {
                state.current_evseId = next_evseId.value();
                return true;
            }

            switch (state.current_source) {
                case ocpp2_0::ChargingLimitSourceEnumType::kValueNotFoundInEnum:
                    return false;

                case ocpp2_0::ChargingLimitSourceEnumType::kEMS:
                    state.current_source = ocpp2_0::ChargingLimitSourceEnumType::kOther;
                    state.current_evseId = 0;
                    break;

                case ocpp2_0::ChargingLimitSourceEnumType::kOther:
                    state.current_source = ocpp2_0::ChargingLimitSourceEnumType::kSO;
                    state.current_evseId = 0;
                    break;

                case ocpp2_0::ChargingLimitSourceEnumType::kSO:
                    state.current_source = ocpp2_0::ChargingLimitSourceEnumType::kCSO;
                    state.current_evseId = 0;
                    break;

                case ocpp2_0::ChargingLimitSourceEnumType::kCSO:
                    return false;
            }

            return true;
        }

        int totalRemainingProfiles(
                ocpp2_0::GetChargingProfilesRequest const& request,
                detail::ReportCharingProfileState const& state
        ) {
            int result = 0;
            detail::ReportCharingProfileState it = state;
            while (advanceState(it))
                result += totalMatchingProfiles(request, it);

            return result;
        }

        std::set<int> getEvseIds() {
            std::set<int> result;
            for (auto const& x : station_->getConnectorMetadata())
                result.insert(x.first.id);

            return result;
        }

    private:
        std::shared_ptr<Settings> settings_;
        std::shared_ptr<SystemInterface> system_;
        std::shared_ptr<StationInterface> station_;
        chargelab::CompressedJournalJson<detail::JournalUpdate> journal_;
        SteadyPointMillis start_timestamp_;

        std::optional<std::pair<SystemTimeMillis, SystemTimeMillis>> last_charging_profile_update_;
        std::optional<SteadyPointMillis> last_reported_journal_capacity_;
        std::optional<std::pair<ocpp2_0::GetChargingProfilesRequest, detail::ReportCharingProfileState>> pending_get_charging_profiles_request_;

        std::map<int, std::optional<SteadyPointMillis>> transaction_start_times_;
        std::map<int, std::optional<std::string>> transaction_id_map_;
        std::vector<ocpp2_0::SetChargingProfileRequest> charging_profiles_;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_POWER_MANAGEMENT_MODULE2_0_H
