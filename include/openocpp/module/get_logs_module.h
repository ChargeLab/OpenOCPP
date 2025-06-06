#ifndef CHARGELAB_OPEN_FIRMWARE_GET_LOGS_MODULE_H
#define CHARGELAB_OPEN_FIRMWARE_GET_LOGS_MODULE_H

#include "openocpp/module/common_templates.h"
#include "openocpp/module/pending_messages_module.h"
#include "openocpp/protocol/common/small_string.h"
#include "openocpp/interface/element/rest_connection_interface.h"
#include "openocpp/interface/platform_interface.h"
#include "openocpp/common/ring_buffer.h"
#include "openocpp/common/serialization.h"

namespace chargelab {
    namespace detail {
        struct UploadState {
            ocpp2_0::GetLogRequest request;
            std::string filename;

            std::shared_ptr<RestConnectionInterface> connection = nullptr;
            int total_failures = 0;

            std::size_t bytes_written = 0;
            std::size_t content_length = 0;
            int min_index = 0;
            int max_index = 0;
            int last_index = 0;

            std::optional<ocpp2_0::UploadLogStatusEnumType> last_status2_0 = std::nullopt;
            std::optional<SteadyPointMillis> first_attempt = std::nullopt;
        };

        struct LogLine {
            int message_index;
            SystemTimeMillis timestamp;
            logging::LogLevel level;
            std::string message;
        };

        class LogLineSerializer {
        public:
            static std::optional<LogLine> read(std::string_view const& text) {
                LogLine result;
                std::optional<int> index = 0;

                index = readPrimitive(text, index, result.message_index);
                index = readPrimitive(text, index, result.timestamp);
                index = readPrimitive(text, index, result.level);
                index = readPrimitive(text, index, result.message);

                if (!index.has_value())
                    return std::nullopt;

                return result;
            }

            static std::string write(LogLine const& wrapper) {
                std::string result;

                writePrimitive(result, wrapper.message_index);
                writePrimitive(result, wrapper.timestamp);
                writePrimitive(result, wrapper.level);
                writePrimitive(result, wrapper.message);

                return result;
            }
        };
    }

    class GetLogsModule : public ServiceStatefulGeneral {
    private:
        // Note: this is the decompressed size of the dropped records
        static constexpr int kTargetDroppedLogMessageBytes = 1024;
        static constexpr int kMaxRetainedLogMessagesBytes = 2*1024;
        static constexpr int kUploadStepMaxTimeMillis = 100;
        static constexpr int kPriorityLogStatusNotification = 100;
        static constexpr int kQueueReportFrequencySeconds = 10;
        static constexpr int kMaxRingBufferSize = 5;

        // Note: arbitrary random assigned ID
        static constexpr std::uint64_t kOperationGroupId = 0xDB0A571F9D6E2A10ull;

    public:
        explicit GetLogsModule(
                std::shared_ptr<PlatformInterface> platform,
                std::shared_ptr<PendingMessagesModule> pending_messages
        )
            : platform_(std::move(platform)),
              pending_messages_(std::move(pending_messages))
        {
            assert(platform_ != nullptr);
            settings_ = platform_->getSettings();
            assert(settings_ != nullptr);

            listener_ = std::make_shared<logging::LoggingListenerFunction>([&](logging::LogMetadata const& metadata, std::string_view const& message) {
                std::string merged_message;
#if defined(LOG_WITH_FILE_AND_LINE)
                merged_message += "[";
                merged_message += metadata.file;
                merged_message += ":";
                merged_message += std::to_string(metadata.line);
                merged_message += "]";
#endif
                merged_message += message;

                log_buffer_.pushBack(detail::LogLine{
                        index_++,
                        platform_->systemClockNow(),
                        metadata.level,
                        std::move(merged_message)
                });
            });

            logging::RegisterLoggingListener(listener_); // register this callback to the global listener

            CHARGELAB_LOG_MESSAGE(info) << "Size of platform pointer: " << sizeof(platform_);
            CHARGELAB_LOG_MESSAGE(info) << "Size of operation: " << sizeof(operation_);
            CHARGELAB_LOG_MESSAGE(info) << "Size of listener: " << sizeof(*listener_);
            CHARGELAB_LOG_MESSAGE(info) << "Size of queue: " << sizeof(log_queue_);
            CHARGELAB_LOG_MESSAGE(info) << "Size of buffer: " << sizeof(log_buffer_);
        }

    private:
        void runUnconditionally() override {
            reportQueueSize();
            uploadLogs();
            flushLogMessages();
        }

        void flushLogMessages() {
            for (int i=0; i < 10; i++) {
                auto next = log_buffer_.popFront();
                if (!next.has_value())
                    break;

                log_queue_.pushBack(std::move(next.value()));
            }

            while (log_queue_.totalBytes() > kMaxRetainedLogMessagesBytes) {
                std::size_t total_removed = 0;
                log_queue_.removeIf([&] (std::string_view const& text, detail::LogLine const&) {
                    if (total_removed > kTargetDroppedLogMessageBytes)
                        return false;

                    total_removed += text.size();
                    return true;
                });
            }
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetLogResponse>>
        onGetLogReq(const ocpp2_0::GetLogRequest& request) override {
            if (!uri::parseHttpUri(request.log.remoteLocation.value()).has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Bad remoteLocation: " << request.log.remoteLocation.value();
                return ocpp2_0::GetLogResponse {ocpp2_0::LogStatusEnumType::kRejected};
            }

            std::string filename = settings_->ChargerSerialNumber.getValue();
            switch (request.logType) {
                case ocpp2_0::LogEnumType::kValueNotFoundInEnum:
                    CHARGELAB_LOG_MESSAGE(warning) << "Bad logType in request";
                    return ocpp2_0::GetLogResponse {ocpp2_0::LogStatusEnumType::kRejected};

                case ocpp2_0::LogEnumType::kDiagnosticsLog:
                    filename += "-diagnostics-";
                    break;

                case ocpp2_0::LogEnumType::kSecurityLog:
                    filename += "-security-";
                    break;
            }
            filename += std::to_string(platform_->systemClockNow()/1000) + ".txt";

            ocpp2_0::LogStatusEnumType status;
            if (!operation_.has_value()) {
                // N01.FR.01
                status = ocpp2_0::LogStatusEnumType::kAccepted;
            } else {
                // N01.FR.12
                status = ocpp2_0::LogStatusEnumType::kAcceptedCanceled;

                // N01.FR.20
                checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kAcceptedCanceled);
            }

            operation_ = detail::UploadState {request, filename};
            return ocpp2_0::GetLogResponse {status, filename};
        }

        std::optional<ocpp1_6::ResponseToRequest<ocpp1_6::TriggerMessageRsp>>
        onTriggerMessageReq(const ocpp1_6::TriggerMessageReq &req) override {
            (void)req;
            return std::nullopt;
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::TriggerMessageResponse>>
        onTriggerMessageReq(const ocpp2_0::TriggerMessageRequest &req) override {
            if (req.requestedMessage != ocpp2_0::MessageTriggerEnumType::kLogStatusNotification)
                return std::nullopt;

            if (req.evse.has_value() || !operation_.has_value()) {
                pending_messages_->sendRequest2_0(
                        ocpp2_0::LogStatusNotificationRequest {
                                ocpp2_0::UploadLogStatusEnumType::kIdle
                        },
                        PendingMessagePolicy {
                                PendingMessageType::kNotificationEvent,
                                kOperationGroupId,
                                settings_->NotificationMessageDefaultRetries.getValue(),
                                settings_->NotificationMessageDefaultRetryInterval.getValue(),
                                kPriorityLogStatusNotification,
                                false,
                                false
                        }
                );
            } else {
                operation_->last_status2_0 = std::nullopt;
                checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kUploading);
            }

            return ocpp2_0::TriggerMessageResponse {ocpp2_0::TriggerMessageStatusEnumType::kAccepted};
        }

    private:
        void reportQueueSize() {
            auto const now = platform_->steadyClockNow();
            if (last_queue_size_report_.has_value()) {
                auto const delta = now - last_queue_size_report_.value();
                if (delta < kQueueReportFrequencySeconds*1000)
                    return;
            }

            last_queue_size_report_ = now;
            CHARGELAB_LOG_MESSAGE(info) << "Total log queue size: " << log_queue_.totalBytes();
        }

        void uploadLogs() {
            if (!operation_.has_value())
                return;

            if ((int)operation_->total_failures > optional::GetOrDefault(operation_->request.retries, 0)) {
                // N01.FR.10
                checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kUploadFailure);
                operation_ = std::nullopt;
                return;
            }

            if (!checkOrRetryConnection())
                return;
            if (!operation_->first_attempt.has_value())
                operation_->first_attempt = platform_->steadyClockNow();

            checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kUploading);

            bool wrote_first = false;
            bool ran_out_of_time = false;
            auto const start_ts = platform_->steadyClockNow();
            {
                bool only_security_logs = operation_->request.logType == ocpp2_0::LogEnumType::kSecurityLog;
                log_queue_.visit([&] (std::string_view const&, detail::LogLine const& line) {
                    if (ran_out_of_time)
                        return;
                    if (operation_->connection == nullptr)
                        return;
                    if (operation_->bytes_written >= operation_->content_length)
                        return;

                    if (wrote_first) {
                        if (platform_->steadyClockNow() - start_ts > kUploadStepMaxTimeMillis) {
                            ran_out_of_time = true;
                            return;
                        }
                    }

                    auto const& log = operation_->request.log;
                    if (log.oldestTimestamp.has_value() && log.oldestTimestamp->isAfter(line.timestamp, false))
                        return;
                    if (log.latestTimestamp.has_value() && log.latestTimestamp->isBefore(line.timestamp, false))
                        return;
                    if (only_security_logs && line.message.find("[security]") == std::string::npos)
                        return;

                    // Note: allowing for integer overflow; checking if message_index < min_index
                    if (line.message_index - operation_->min_index < 0)
                        return;

                    // Note: allowing for integer overflow; checking if message_index > max_index
                    if (line.message_index - operation_->max_index > 0)
                        return;

                    // Note: allowing for integer overflow; checking if message_index <= last_index
                    if (line.message_index - operation_->last_index <= 0)
                        return;

                    // Truncate the message to fit in the original content_length
                    auto message = renderLine(line);
                    if (operation_->bytes_written + message.size() > operation_->content_length) {
                        auto const truncated = operation_->bytes_written + message.size() - operation_->content_length;
                        message.resize(std::min(std::max((std::size_t)0, truncated), message.size()));
                    }

                    if (!operation_->connection->write(message.data(), message.size())) {
                        CHARGELAB_LOG_MESSAGE(info) << "Failed writing payload at " << operation_->bytes_written << " bytes";
                        checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kUploadFailure);
                        operation_->total_failures++;
                        operation_->connection = nullptr;
                        return;
                    }

                    operation_->last_index = line.message_index;
                    operation_->bytes_written += message.size();
                    wrote_first = true;
                });
            }

            if (ran_out_of_time)
                return;
            if (operation_->connection == nullptr)
                return;

            std::string buffer;
            while (operation_->bytes_written < operation_->content_length) {
                // Note: this can happen if log lines are removed while an upload operation is in progress
                buffer.resize(std::min((std::size_t)1024, operation_->content_length - operation_->bytes_written), ' ');

                if (!operation_->connection->write(buffer.data(), buffer.size())) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed writing padding to server";
                    checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kUploadFailure);
                    operation_->total_failures++;
                    operation_->connection = nullptr;
                    return;
                }

                CHARGELAB_LOG_MESSAGE(warning) << "Added padding: " << buffer.size() << " bytes";
                operation_->bytes_written += buffer.size();
            }

            if (!operation_->connection->send()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed finishing HTTP request";
                checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kUploadFailure);
                operation_->total_failures++;
                operation_->connection = nullptr;
                return;
            }

            auto const status = operation_->connection->getStatusCode();
            if (status == 403) {
                CHARGELAB_LOG_MESSAGE(warning) << "Unauthorized response from server: " << status;
                // N01.FR.10
                checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kPermissionDenied);
                operation_->total_failures++;
                operation_->connection = nullptr;
                return;
            }
            if (status < 200 || status >= 300) {
                CHARGELAB_LOG_MESSAGE(warning) << "Unexpected status code: " << status;
                // N01.FR.10
                checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kBadMessage);
                operation_->total_failures++;
                operation_->connection = nullptr;
                return;
            }

            CHARGELAB_LOG_MESSAGE(warning) << "Upload succeeded - response was: " << status;
            checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kUploaded);
            operation_ = std::nullopt;
        }

        static std::string renderLine(detail::LogLine const& line) {
            std::string result;
            result += "[" + write_json_to_string(ocpp2_0::DateTime{line.timestamp}) + "] ";
            result += "[" + write_json_to_string(line.level) + "] ";
            result += line.message;
            result += "\n";
            return result;
        }

        bool checkOrRetryConnection() {
            if (!operation_.has_value())
                return false;
            if (operation_->connection != nullptr)
                return true;

            if (operation_->first_attempt.has_value()) {
                auto const delta_seconds = (platform_->steadyClockNow() - operation_->first_attempt.value())/1000;
                auto const interval = optional::GetOrDefault(operation_->request.retryInterval, 0);
                if (delta_seconds < interval*operation_->total_failures)
                    return false;
            }

            auto const uri = operation_->request.log.remoteLocation.value();
            CHARGELAB_LOG_MESSAGE(info) << "Uploading logs to: " << uri;
            operation_->connection = platform_->putRequest(uri);
            if (operation_->connection == nullptr) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed creating post request for: " << uri;
                // N01.FR.10
                checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kUploadFailure);
                operation_->total_failures++;
                return false;
            }

            // Compute the size and index limits
            std::optional<int> min_index = std::nullopt;
            std::optional<int> max_index = std::nullopt;
            std::size_t content_length = 0;
            {
                log_queue_.visit([&] (std::string_view const&, detail::LogLine const& line) {
                    auto const& log = operation_->request.log;
                    if (log.oldestTimestamp.has_value() && log.oldestTimestamp->isAfter(line.timestamp, false))
                        return;
                    if (log.latestTimestamp.has_value() && log.latestTimestamp->isBefore(line.timestamp, false))
                        return;

                    if (!min_index.has_value()) {
                        min_index = line.message_index;
                    } else {
                        // Note: allowing for integer overflow; checking if message_index < min_index
                        if (line.message_index - min_index.value() < 0)
                            min_index = line.message_index;
                    }

                    if (!max_index.has_value()) {
                        max_index = line.message_index;
                    } else {
                        // Note: allowing for integer overflow; checking if message_index > max_index
                        if (line.message_index - max_index.value() > 0)
                            max_index = line.message_index;
                    }

                    content_length += renderLine(line).size();
                });
            }

            operation_->bytes_written = 0;
            operation_->content_length = content_length;
            operation_->min_index = optional::GetOrDefault(min_index, 0);
            operation_->max_index = optional::GetOrDefault(max_index, 0);
            operation_->last_index = optional::GetOrDefault(min_index, 0) - 1;

            // N01.FR.19
            operation_->connection->setHeader("Content-Type", "text/plain");
            operation_->connection->setHeader("Content-Disposition", operation_->filename);

            if (!operation_->connection->open(content_length)) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed opening connection to server";
                checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType::kUploadFailure);
                operation_->total_failures++;
                operation_->connection = nullptr;
            } else {
                CHARGELAB_LOG_MESSAGE(info) << "Connection opened to server - uploading " << content_length << " bytes...";
            }

            return true;
        }

        void checkAndUpdateStatus(ocpp2_0::UploadLogStatusEnumType status) {
            if (operation_->last_status2_0 == std::make_optional(status))
                return;

            pending_messages_->sendRequest2_0(
                    ocpp2_0::LogStatusNotificationRequest {
                            status,
                            operation_->request.requestId
                    },
                    PendingMessagePolicy {
                            PendingMessageType::kNotificationEvent,
                            kOperationGroupId,
                            settings_->NotificationMessageDefaultRetries.getValue(),
                            settings_->NotificationMessageDefaultRetryInterval.getValue(),
                            kPriorityLogStatusNotification,
                            false,
                            false
                    }
            );

            operation_->last_status2_0 = status;
        }

    private:
        std::shared_ptr<PlatformInterface> platform_;
        std::shared_ptr<PendingMessagesModule> pending_messages_;
        std::shared_ptr<Settings> settings_;
        std::shared_ptr<logging::LoggingListenerFunction> listener_;

        std::optional<detail::UploadState> operation_ = std::nullopt;
        std::optional<SteadyPointMillis> last_queue_size_report_ = std::nullopt;
        RingBuffer<detail::LogLine, kMaxRingBufferSize> log_buffer_;

        std::atomic<int> index_ = 0;
        CompressedQueueCustom<detail::LogLine, detail::LogLineSerializer> log_queue_;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_GET_LOGS_MODULE_H
