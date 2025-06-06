#ifndef CHARGELAB_OPEN_FIRMWARE_OPERATION_HOLDER_H
#define CHARGELAB_OPEN_FIRMWARE_OPERATION_HOLDER_H

#include "openocpp/interface/component/system_interface.h"
#include "openocpp/common/logging.h"

#include <optional>
#include <memory>

namespace chargelab {
    struct NoOperation {};
    inline constexpr NoOperation kNoOperation;

    template <typename T>
    class OperationHolder {
    public:
        explicit OperationHolder(std::shared_ptr<SystemInterface> system)
            : system_(std::move(system))
        {
            assert(system_ != nullptr);
        }

        OperationHolder& operator=(NoOperation const&) {
            state_ = std::nullopt;
            consecutive_failures_ = std::nullopt;
            idle_since_ = system_->steadyClockNow();
            return *this;
        }

        bool operator==(NoOperation const&) const {
            return !operationInProgress();
        }

        bool operator==(T const& rhs) const {
            if (!state_.has_value())
                return false;

            return state_->first == rhs && operationInProgress();
        }

        bool operator!=(NoOperation const& rhs) const {
            return !operator==(rhs);
        }

        bool operator!=(T const& rhs) const {
            return !operator==(rhs);
        }

        [[nodiscard]] int consecutiveFailures() const {
            int consecutive_failures = 0;
            if (consecutive_failures_.has_value()) {
                consecutive_failures = consecutive_failures_.value();
            }
            if (currentOperationTimedOut()) {
                consecutive_failures++;
            }

            return consecutive_failures;
        }

        [[nodiscard]] bool currentOperationTimedOut() const {
            return state_.has_value() && !operationInProgress();
        }

        [[nodiscard]] bool operationInProgress() const {
            if (!state_.has_value())
                return false;

            auto const delta = system_->steadyClockNow() - state_->second;
            if (delta < 0) {
                CHARGELAB_LOG_MESSAGE(error) << "Invalid steady clock delta: " << delta;
                return false;
            } else if (delta >= timeout_seconds_*1000) {
                return false;
            }

            return true;
        }

        [[nodiscard]] int getIdleDurationSeconds() const {
            if (!state_.has_value()) {
                if (idle_since_.has_value()) {
                    return (system_->steadyClockNow() - idle_since_.value())/1000;
                } else {
                    return std::numeric_limits<int>::max();
                }
            }

            auto const delta = (system_->steadyClockNow() - state_->second)/1000 - timeout_seconds_;
            if (delta < 0)
                return 0;
            if (delta >= std::numeric_limits<int>::max())
                return std::numeric_limits<int>::max();

            return delta;
        }

        [[nodiscard]] bool wasIdleFor(int seconds) {
            return !operationInProgress() && getIdleDurationSeconds() >= seconds;
        }

        void setWithTimeout(int timeout_seconds, std::optional<T> const& id) {
            timeout_seconds_ = timeout_seconds;

            if (id.has_value()) {
                state_ = std::make_pair(id.value(), system_->steadyClockNow());

                if (consecutive_failures_.has_value()) {
                    consecutive_failures_ = consecutive_failures_.value() + 1;
                } else {
                    consecutive_failures_ = 0;
                }
            } else {
                state_ = std::nullopt;
                consecutive_failures_ = std::nullopt;
            }
        }

    private:
        std::shared_ptr<SystemInterface> system_;
        int timeout_seconds_;

        std::optional<std::pair<T, SteadyPointMillis>> state_ = std::nullopt;
        std::optional<int> consecutive_failures_ = std::nullopt;
        std::optional<SteadyPointMillis> idle_since_ = std::nullopt;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_OPERATION_HOLDER_H
