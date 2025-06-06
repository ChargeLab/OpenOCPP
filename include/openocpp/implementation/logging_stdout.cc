#include "openocpp/common/logging.h"
#include "openocpp/helpers/chrono.h"
#include "openocpp/helpers/optional.h"

#include <iostream>
#include <atomic>
#include <chrono>
#include <mutex>

namespace chargelab::logging {
    namespace {
        std::atomic<LogLevel> gLogLevel {LogLevel::trace};

        std::atomic<int> gRecursiveCounter {};
        std::recursive_mutex gMutex {};
        std::vector<std::shared_ptr<LoggingListenerFunction>> gListeners {};

        class RaiiCounter {
        public:
            explicit RaiiCounter(std::atomic<int>& counter) : counter_{counter} {
                counter_++;
            }

            ~RaiiCounter() {
                counter_--;
            }

        private:
            std::atomic<int>& counter_;
        };
    }

    std::string_view ToString(LogLevel const& level) {
        switch (level) {
            case LogLevel::trace: return std::string_view {"trace"};
            case LogLevel::debug: return std::string_view {"debug"};
            case LogLevel::info: return std::string_view {"info"};
            case LogLevel::warning: return std::string_view {"warning"};
            case LogLevel::error: return std::string_view {"error"};

            default:
            case LogLevel::fatal: return std::string_view {"fatal"};
        }
    }

    void SetLogLevel(LogLevel level) {
        gLogLevel = level;
    }

    bool IsLoggingEnabled(LogLevel level) {
        LogLevel const limit =  gLogLevel;
        return static_cast<int>(level) >= static_cast<int>(limit);
    }

    std::ostream& LogPrefix(LogMetadata const& metadata) {
        auto const now = std::chrono::system_clock::now();
        auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        auto const ts = chrono::ToString(static_cast<chargelab::SystemTimeMillis>(millis));

        std::ios_base::fmtflags original_flags(std::cout.flags());


        std::cout << "[" << optional::GetOrDefault<std::string>(ts, "<bad timestamp>")
                         << "] [" << std::hex << std::this_thread::get_id()
                         << "] [" << ToString(metadata.level)
#if defined(LOG_WITH_FILE_AND_LINE)
                         << "] [" << metadata.file << ":" << metadata.line << "(" << metadata.function << ")] ";
#else
                         << "] [" << "(" << metadata.function << ")] ";
#endif

        std::cout.flags(original_flags);
        return std::cout;
    }

    void PrintLogMessage(LogMetadata const& metadata, std::string const& message) {
        LogPrefix(metadata) << message << std::endl;

        std::lock_guard lock {gMutex};
        RaiiCounter counter {gRecursiveCounter};
        if (gRecursiveCounter > 2) {
            LogPrefix(metadata) << "Recursive PrintLogMessage call - suppressing messages" << std::endl;
            return;
        }

        if (!gListeners.empty()) {
            for (auto const& x : gListeners) {
                if (x != nullptr) {
                    (*x)(metadata, message);
                }
            }
        }
    }

    void PrintLogMessage(LogMetadata const& metadata, std::string_view const& message) {
        LogPrefix(metadata) << message << std::endl;

        std::lock_guard lock {gMutex};
        RaiiCounter counter {gRecursiveCounter};
        if (gRecursiveCounter > 2) {
            LogPrefix(metadata) << "Recursive PrintLogMessage call - suppressing messages" << std::endl;
            return;
        }

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