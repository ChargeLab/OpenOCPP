#ifndef CHARGELAB_OPEN_FIRMWARE_LOG_STREAMING_MODULE_H
#define CHARGELAB_OPEN_FIRMWARE_LOG_STREAMING_MODULE_H

#include "openocpp/module/common_templates.h"
#include "openocpp/interface/component/system_interface.h"
#include "openocpp/common/settings.h"

#include <utility>
#include <sstream>

namespace chargelab {
    namespace detail {
        struct LogLine {
            LogLevel level = LogLevel::kValueNotFoundInEnum;
#if defined(LOG_WITH_FILE_AND_LINE)
            std::string file {};
            int line = -1;
#endif
            std::string function {};
            std::string message {};

            int size() {
                return sizeof(LogLine) +
#if defined(LOG_WITH_FILE_AND_LINE)
                file.size() +
#endif
                function.size() + message.size();
            }
        };
#if defined(LOG_WITH_FILE_AND_LINE)
        CHARGELAB_OPTIONAL_NULL_DEFINE_TYPE_NON_INTRUSIVE(LogLine, level, file, line, function, message)
#else
        CHARGELAB_OPTIONAL_NULL_DEFINE_TYPE_NON_INTRUSIVE(LogLine, level, function, message)
#endif
        struct LogMessage {
            std::vector<LogLine> messages;
        };
        CHARGELAB_OPTIONAL_NULL_DEFINE_TYPE_NON_INTRUSIVE(LogMessage, messages)
    }

    class LogStreamingModule : public ChargePointServiceStateful {
    private:
        static const int kBufferLimitBytes = 3*1024; // 10*1024;
        static const int kRingBufferMaxSize = 25; // 50;
        static const int kMaxLinesPerMessage = 15; // 20;
        static const int kBufferLimitMillis = 30*1000; // 30*1000;
    public:
        LogStreamingModule(
                Settings& settings,
                std::shared_ptr<SystemInterface> const& system_interface
        ) : system_interface_(system_interface),
            pending_dump_request_ {system_interface},
            log_streaming_enabled_ {chargelab::settings::CreateBasicBooleanSetting<false>(
                    settings,
                    settings::CommonKey::kLogStreamingEnabled,
                    "LogStreamingEnabled"
            )}
        {
            listener_ = std::make_shared<LoggingListenerFunction>([&](LogMetadata const& metadata, std::string_view const& message) {
                // TODO: Find a better way to filter out specific messages related to the dump itself
                if (message.find("Writing text message") != std::string::npos && message.find("StreamingLogMessage") != std::string::npos) {
                    return;
                }
                if (!log_streaming_enabled_.getValue())
                    return;

                detail::LogLine line {};
                line.level = metadata.level;
#if defined(LOG_WITH_FILE_AND_LINE)
                line.file = metadata.file;
                line.line = metadata.line;
                line.file.shrink_to_fit();
#endif
                line.function = metadata.function;
                line.message = message;

                line.function.shrink_to_fit();
                line.message.shrink_to_fit();

                std::lock_guard lock{mutex_};
                history_.pushBack(std::move(line));
            });

            RegisterLoggingListener(listener_);
        }

        ~LogStreamingModule() override {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting LogStreamingModule";
            UnregisterLoggingListener(listener_);
        }

    private:
        void runStep(ocpp1_6::ChargePointRemoteInterface &remote) override {
            {
                std::lock_guard lock{mutex_};
                if (history_.empty())
                    return;

                auto const now = system_interface_->steadyClockNow();
                if (history_.size() < kMaxLinesPerMessage && !getHistoryLimitIndex().has_value()) {
                    if (!last_dump_.has_value())
                        last_dump_ = now;

                    auto const delta = now - last_dump_.value();
                    if (delta < kBufferLimitMillis) {
                        return;
                    }
                }
            }

            if (!pending_dump_request_.operationInProgress()) {
                CHARGELAB_LOG_MESSAGE(debug) << "Flushing log messages - total size was: " << getHistorySize() << " bytes" << ", total memory size: " << getHistoryMemorySize()
                    << ",ring buffer size: " << history_.size();

                detail::LogMessage payload {};
                {
                    std::lock_guard lock{mutex_};
                    int total_size = 0;
                    for (int i=0; i < kMaxLinesPerMessage && !history_.empty(); i++) {
                        if (total_size + history_.front().size() > kBufferLimitBytes) break;
                        total_size += history_.front().size();

                        detail::LogLine value = std::move(history_.front());

                        history_.popFront();

                        payload.messages.push_back(std::move(value));
                    }
                }

                pending_dump_request_.setWithTimeout(
                        settings_->DefaultMessageTimeout.getValue(),
                        remote.sendDataTransferReq(ocpp1_6::DataTransferReq {
                                ocpp1_6::CiString255Type("ChargeLab"),
                                ocpp1_6::CiString50Type("StreamingLogMessage"),
                                write_json_to_string(payload)
                        })
                );
            }

            std::lock_guard lock{mutex_};
            auto limit = getHistoryLimitIndex();
            if (limit.has_value()) {
                for (int i=0; i <= limit.value(); i++) {
                    history_.front() = detail::LogLine {};
                    history_.popFront();
                }
            }
        }

        void onDataTransferRsp(
                const std::string &unique_id,
                const ocpp1_6::ResponseMessage <chargelab::ocpp1_6::DataTransferRsp> &rsp
        ) override {
            if (pending_dump_request_ == unique_id) {
                pending_dump_request_ = kNoOperation;
                last_dump_ = system_interface_->steadyClockNow();

                if (std::holds_alternative<ocpp1_6::CallError>(rsp)) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Error response to DataTransfer request: " << std::get<ocpp1_6::CallError> (rsp);
                }
            }
        }

    private:
        std::optional<int> getHistoryLimitIndex() {
            int index;
            std::size_t total_bytes = 0;
            for (index = history_.size()-1; index >= 0; index--) {
                auto const& value = history_[index];
                total_bytes += sizeof(value.level);
#if defined(LOG_WITH_FILE_AND_LINE)
                total_bytes += sizeof(std::string) + value.file.size();
                total_bytes += sizeof(value.line);
#endif
                total_bytes += sizeof(std::string) + value.function.size();
                total_bytes += sizeof(std::string) + value.message.size();

                if (total_bytes >= kBufferLimitBytes)
                    return index;
            }

            return std::nullopt;
        }

        std::size_t getHistorySize() {
            std::size_t total_bytes = 0;
            for (int index=0; index < history_.size(); index++) {
                auto const& value = history_[index];
                total_bytes += sizeof(value.level);
#if defined(LOG_WITH_FILE_AND_LINE)
                total_bytes += sizeof(std::string) + value.file.size();
                total_bytes += sizeof(value.line);
#endif
                total_bytes += sizeof(std::string) + value.function.size();
                total_bytes += sizeof(std::string) + value.message.size();
            }

            return total_bytes;
        }

        std::size_t getHistoryMemorySize() {
            std::size_t total_bytes = 0;
            for (int index=0; index < history_.size(); index++) {
                auto const& value = history_[index];
                total_bytes += sizeof(value.level);
#if defined(LOG_WITH_FILE_AND_LINE)
                total_bytes += sizeof(std::string) + value.file.capacity();
                total_bytes += sizeof(value.line);
#endif
                total_bytes += sizeof(std::string) + value.function.capacity();
                total_bytes += sizeof(std::string) + value.message.capacity();
            }

            return total_bytes;
        }

    private:
        std::shared_ptr<SystemInterface> system_interface_;
        OperationHolder<std::string> pending_dump_request_;
        BasicBooleanSetting<false> log_streaming_enabled_;

        std::optional<SteadyPointMillis> last_dump_ = std::nullopt;
        std::shared_ptr<LoggingListenerFunction> listener_ = nullptr;

        std::mutex mutex_;
        RingBuffer<detail::LogLine, kRingBufferMaxSize> history_;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_LOG_STREAMING_MODULE_H
