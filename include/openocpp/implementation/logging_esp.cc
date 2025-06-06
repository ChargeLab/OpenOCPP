#include "openocpp/common/logging.h"

#include <atomic>
#include <mutex>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

namespace chargelab::logging {
    namespace {
        std::atomic<LogLevel> gLogLevel {LogLevel::trace};
        constexpr char const* kTag = "main";

        std::recursive_mutex gMutex {};
        std::vector<std::shared_ptr<LoggingListenerFunction>> gListeners {};
    }

    void SetLogLevel(LogLevel level) {
        gLogLevel = level;

        switch (level) {
            case LogLevel::trace: esp_log_level_set(kTag, ESP_LOG_VERBOSE); break;
            case LogLevel::debug: esp_log_level_set(kTag, ESP_LOG_DEBUG); break;

            default:
            case LogLevel::info: esp_log_level_set(kTag, ESP_LOG_INFO); break;

            case LogLevel::warning: esp_log_level_set(kTag, ESP_LOG_WARN); break;
            case LogLevel::error: esp_log_level_set(kTag, ESP_LOG_ERROR); break;
            case LogLevel::fatal: esp_log_level_set(kTag, ESP_LOG_ERROR); break;
        }
    }

    bool IsLoggingEnabled(LogLevel level) {
        LogLevel const limit =  gLogLevel;
        return static_cast<int>(level) >= static_cast<int>(limit);
    }
#if defined(LOG_WITH_FILE_AND_LINE)
#define CHARGELAB_ESP_LOGGING_TEMPLATE(esp_macro) esp_macro(kTag, "[%s:%d(%s)] %s", metadata.file.data(), metadata.line, metadata.function.data(), message)
#else
#define CHARGELAB_ESP_LOGGING_TEMPLATE(esp_macro) esp_macro(kTag, "[%s] %s", metadata.function.data(), message)
#endif

    namespace detail {
        void PrintLogMessage(LogMetadata const& metadata, char const* message) {
            switch (metadata.level) {
                case LogLevel::trace: CHARGELAB_ESP_LOGGING_TEMPLATE(ESP_LOGV); break;
                case LogLevel::debug: CHARGELAB_ESP_LOGGING_TEMPLATE(ESP_LOGD); break;
                case LogLevel::info: CHARGELAB_ESP_LOGGING_TEMPLATE(ESP_LOGI); break;
                case LogLevel::warning: CHARGELAB_ESP_LOGGING_TEMPLATE(ESP_LOGW); break;
                case LogLevel::error: CHARGELAB_ESP_LOGGING_TEMPLATE(ESP_LOGE); break;

                default:
                case LogLevel::fatal: CHARGELAB_ESP_LOGGING_TEMPLATE(ESP_LOGE); break;
            }
        }
    }

#undef CHARGELAB_ESP_LOGGING_TEMPLATE

    void PrintLogMessage(LogMetadata const& metadata, std::string const& message) {
        detail::PrintLogMessage(metadata, message.data());

        std::lock_guard lock {gMutex};
        if (!gListeners.empty()) {
            for (auto const& x : gListeners) {
                if (x != nullptr) {
                    (*x)(metadata, message);
                }
            }
        }
    }

    void PrintLogMessage(LogMetadata const& metadata, std::string_view const& message) {
        detail::PrintLogMessage(metadata, message.data());

        std::lock_guard lock {gMutex};
        if (!gListeners.empty()) {
            for (auto const& x : gListeners) {
                if (x != nullptr) {
                    (*x)(metadata, message);
                }
            }
        }
    }

    void RegisterLoggingListener(std::shared_ptr<LoggingListenerFunction> const& callback) {
        std::lock_guard lock {gMutex};
        for (auto const& x : gListeners) {
            if (x == callback) {
                CHARGELAB_LOG_MESSAGE(warning) << "Callback registered multiple times";
                return;
            }
        }

        gListeners.push_back(callback);
    }

    void UnregisterLoggingListener(std::shared_ptr<LoggingListenerFunction> const& callback) {
        std::lock_guard lock {gMutex};

        auto initial_size = gListeners.size();
        gListeners.erase(
                std::remove_if(gListeners.begin(), gListeners.end(), [&](auto& x) {return x == callback;}),
                gListeners.end()
        );

        if (initial_size - gListeners.size() > 1)
            CHARGELAB_LOG_MESSAGE(warning) << "Callback registered multiple times";
    }
}