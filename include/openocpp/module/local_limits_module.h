#ifndef CHARGELAB_OPEN_FIRMWARE_LOCAL_LIMITS_MODULE_H
#define CHARGELAB_OPEN_FIRMWARE_LOCAL_LIMITS_MODULE_H

#include "openocpp/interface/component/system_interface.h"

#include <vector>
#include <string>
#include <optional>
#include <iostream>

namespace chargelab {
    struct LeaseAmps {
        int created_index;
        double allocated_amps;
        SteadyPointMillis expiry;

        std::optional<int> completed_index = std::nullopt;
    };

    namespace detail {
        struct LocalLimitState {
            std::string charge_point_id_;
            std::vector<LeaseAmps> active_leases_;

            std::optional<double> requested_amps_;
            std::optional<SteadyPointMillis> requested_amps_ts_;

            // Informational
            std::optional<double> last_reported_amps_;
            std::optional<double> last_reported_watts_;
        };
    }

    class LocalLimitModule {
    private:
        static constexpr int kRequestExpirySeconds = 3600;
        static constexpr int kLeaseDurationSeconds = 3600;
        static constexpr int kLeaseRefreshThresholdSeconds = 15*60;

    public:
        explicit LocalLimitModule(
                std::shared_ptr<chargelab::SystemInterface> system,
                double max_amps
        )
                : system_(std::move(system)),
                  max_amps_(max_amps)
        {
        }

    public:
        int getUniqueId(std::string const& charge_point_id) {
            std::lock_guard lock {mutex_};
            for (auto const& x : state_) {
                if (x.second.charge_point_id_ == charge_point_id)
                    return x.first;
            }

            auto result = last_id_++;
            state_[result].charge_point_id_ = charge_point_id;
            return result;
        }

        void setRequestedAmps(int id, double amps) {
            std::lock_guard lock {mutex_};
            state_[id].requested_amps_ = amps;
            state_[id].requested_amps_ts_ = system_->steadyClockNow();
        }

        void setCurrentAllocatedAmps(int id, double amps) {
            std::lock_guard lock {mutex_};
            state_[id].last_reported_amps_ = amps;
        }

        void setCurrentWatts(int id, double watts) {
            std::lock_guard lock {mutex_};
            state_[id].last_reported_watts_ = watts;
        }

        std::optional<LeaseAmps> leaseAmps(int id, std::optional<LeaseAmps> const& current) {
            std::lock_guard lock {mutex_};
            auto result = leaseAmpsImpl(id);

            bool refresh = false;
            if (result.allocated_amps != current->allocated_amps)
                refresh = true;
            if ((current->expiry - system_->steadyClockNow())/1000 < kLeaseRefreshThresholdSeconds)
                refresh = true;

            if (!refresh)
                return std::nullopt;

            state_[id].active_leases_.push_back(result);
            return result;
        }

        void activateLease(int id, LeaseAmps& current) {
            std::lock_guard lock {mutex_};

            int completed_index = lease_index_++;
            current.completed_index = completed_index;

            auto& leases = state_[id].active_leases_;
            for (auto& x : leases) {
                if (x.created_index == current.created_index) {
                    x.completed_index = completed_index;
                    break;
                }
            }

            leases.erase(
                    std::remove_if(
                            leases.begin(),
                            leases.end(),
                            [=](LeaseAmps const& x){return x.completed_index && current.created_index - x.completed_index.value() > 0;}
                    ),
                    leases.end()
            );
        }

        void printCurrentAmps() {
            std::lock_guard lock {mutex_};

            system("clear");

            auto now = system_->steadyClockNow();
            for (auto& x : state_) {
                auto lease = removeDeadEntriesAndGetMaxLease(x.second.active_leases_);
                if (!lease)
                    continue;

                std::cout << "ID " << x.first << " {" << x.second.charge_point_id_ << "}: ";
                std::cout << std::setw(4) << std::round(lease->allocated_amps*10)/10.0 << "A (" << std::setw(2) << static_cast<int>((lease->expiry - now)/1000) << "s)";
                if (x.second.last_reported_amps_ && x.second.requested_amps_)
                    std::cout << " " << std::setw(4) << std::round(x.second.last_reported_amps_.value()*10)/10.0 << "A/" << std::setw(4) << std::round(x.second.requested_amps_.value()*10)/10.0 << "A";
                if (x.second.last_reported_watts_)
                    std::cout << " " <<  std::setw(4) << std::round(x.second.last_reported_watts_.value()) << "W";
                std::cout << std::endl;
            }
        }

    private:
        LeaseAmps leaseAmpsImpl(int id) {
            auto now = system_->steadyClockNow();
            auto expiry = static_cast<SteadyPointMillis>(now + kLeaseDurationSeconds*1000);
            int index = lease_index_++;

            auto const& state = state_[id];
            if (!state.requested_amps_)
                return {index, 0.0, expiry};
            if (!state.requested_amps_ts_ || (now - state.requested_amps_ts_.value())/1000.0 >= kRequestExpirySeconds)
                return {index, 0.0, expiry};

            double total_requested = 0;
            for (auto const& x : state_) {
                if (!x.second.requested_amps_ || !x.second.requested_amps_ts_)
                    continue;
                if ((now - x.second.requested_amps_ts_.value())/1000.0 >= kRequestExpirySeconds)
                    continue;

                total_requested += x.second.requested_amps_.value();
            }

            double total_leased = 0;
            for (auto& x : state_) {
                if (x.first == id)
                    continue;

                auto lease = removeDeadEntriesAndGetMaxLease(x.second.active_leases_);
                if (lease)
                    total_leased += lease->allocated_amps;
            }

            auto requested = state.requested_amps_.value();
            auto result = std::min(requested, requested*max_amps_/total_requested);
            result = std::min(max_amps_ - total_leased, result);
            result = std::max(0.0, result);

            return {index, result, expiry};
        }

        std::optional<LeaseAmps> removeDeadEntriesAndGetMaxLease(std::vector<LeaseAmps>& leases) {
            auto now = system_->steadyClockNow();
            leases.erase(
                    std::remove_if(
                            leases.begin(),
                            leases.end(),
                            [&](auto const& x) {return now - x.expiry > 0;}
                    ),
                    leases.end()
            );

            std::optional<LeaseAmps> max_lease;
            for (auto const& x : leases) {
                if (max_lease) {
                    if (x.allocated_amps < max_lease->allocated_amps)
                        continue;
                    if (x.allocated_amps == max_lease->allocated_amps && x.expiry - max_lease->expiry < 0)
                        continue;
                }

                max_lease = x;
            }

            return max_lease;
        }

    private:
        std::shared_ptr<chargelab::SystemInterface> system_;
        double max_amps_;
        int last_id_ = 0;
        int lease_index_ = 0;

        std::mutex mutex_;
        std::map<int, detail::LocalLimitState> state_;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_LOCAL_LIMITS_MODULE_H
