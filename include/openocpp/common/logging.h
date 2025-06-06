#ifndef CHARGELAB_OPEN_FIRMWARE_LOGGING_H
#define CHARGELAB_OPEN_FIRMWARE_LOGGING_H

#include "openocpp/model/system_types.h"

#include <atomic>
#include <variant>
#include <optional>
#include <thread>
#include <memory>
#include <functional>
#include <string_view>
#include <string>

namespace chargelab::logging {
    enum class LogLevel {
        trace,
        debug,
        info,
        warning,
        error,
        fatal
    };

    struct LogMetadata {
        LogLevel level;
#if defined(LOG_WITH_FILE_AND_LINE)
        std::string_view file;
        int line;
#endif
        std::string_view function;
    };

    void SetLogLevel(LogLevel level);
    bool IsLoggingEnabled(LogLevel level);
    void PrintLogMessage(LogMetadata const& metadata, std::string_view const& message);
    void PrintLogMessage(LogMetadata const& metadata, std::string const& message);

    using LoggingListenerFunction = std::function<void(LogMetadata const& metadata, std::string_view const& message)>;
    void RegisterLoggingListener(std::shared_ptr<LoggingListenerFunction> const& callback);
    void UnregisterLoggingListener(std::shared_ptr<LoggingListenerFunction> const& callback);

    template <typename T, typename U=void>
    struct LogWriter;

    template <>
    struct LogWriter<std::string, void> {
        static void write(std::string& accumulator, std::string const& value) {
            accumulator += value;
        }
    };

    template <>
    struct LogWriter<std::string_view, void> {
        static void write(std::string& accumulator, std::string_view const& value) {
            accumulator += value;
        }
    };

    template <>
    struct LogWriter<char const*, void> {
        static void write(std::string& accumulator, char const* value) {
            accumulator += value;
        }
    };

    template <int N>
    struct LogWriter<char const[N], void> {
        static void write(std::string& accumulator, char const* value) {
            accumulator += value;
        }
    };

    template <int N>
    struct LogWriter<char[N], void> {
        static void write(std::string& accumulator, char const* value) {
            accumulator += value;
        }
    };

    template <typename T>
    struct LogWriter<T, typename std::enable_if<std::is_arithmetic<T>::value>::type> {
        static void write(std::string& accumulator, T const& value) {
            accumulator += std::to_string(value);
        }
    };

    template <>
    struct LogWriter<SystemTimeMillis, void> {
        static void write(std::string& accumulator, SystemTimeMillis value) {
            accumulator += std::to_string(static_cast<int64_t>(value));
        }
    };

    template <>
    struct LogWriter<SteadyPointMillis, void> {
        static void write(std::string& accumulator, SteadyPointMillis value) {
            accumulator += std::to_string(static_cast<int64_t>(value));
        }
    };

    template <typename T>
    struct LogWriter<T, typename std::enable_if_t<std::is_enum<T>::value>> {
        static void write(std::string& accumulator, T const& value) {
            using IntType = typename std::underlying_type<T>::type;
            accumulator += std::to_string(static_cast<IntType>(value));
        }
    };

    class LogAccumulator {
    public:
        explicit LogAccumulator(LogLevel level,
#if defined(LOG_WITH_FILE_AND_LINE)
                                std::string_view file, int line,
#endif
                                std::string_view function)
            : metadata_ {level,
#if defined(LOG_WITH_FILE_AND_LINE)
                         file, line,
#endif
                         function},
              enabled_(IsLoggingEnabled(level))
        {
        }

        ~LogAccumulator() {
            PrintLogMessage(metadata_, accumulator_);
        }

        [[nodiscard]] bool getDone() const {
            return !enabled_ || done_;
        }

        void setDone(bool done) {
            done_ = done;
        }

        template <typename T>
        friend LogAccumulator& operator<<(LogAccumulator& os, T const& value) {
            LogWriter<T>::write(os.accumulator_, value);
            return os;
        }

    private:
        LogMetadata metadata_;
        bool enabled_;

        bool done_ = false;
        std::string accumulator_;
    };

    namespace detail {
        constexpr char const* FileName(char const* path) {
            char const* it = path;
            char const* result = it;
            while (true) {
                auto ch = *(it++);
                if (ch == '\0')
                    break;
                if (ch == '/')
                    result = it;
            }

            return result;
        }
    }


    class NoOpAccumulator {
        template <typename T>
        friend NoOpAccumulator& operator<<(NoOpAccumulator& os, T const&) {
            return os;
        }
    };
}

#ifndef CHARGELAB_DISABLE_LOGGING

#if defined(LOG_WITH_FILE_AND_LINE)
#define CHARGELAB_LOG_MESSAGE(level) \
    for (::chargelab::LogAccumulator accumulator{::chargelab::LogLevel::level, ::chargelab::detail::FileName(__FILE__), __LINE__, __func__}; !accumulator.getDone(); accumulator.setDone(true)) \
        accumulator
#else
#define CHARGELAB_LOG_MESSAGE(level) \
    for (::chargelab::logging::LogAccumulator accumulator{::chargelab::logging::LogLevel::level, __func__}; !accumulator.getDone(); accumulator.setDone(true)) \
        accumulator
#endif

#else
#define CHARGELAB_LOG_MESSAGE(level) \
    for (::chargelab::NoOpAccumulator accumulator{}; false;) \
        accumulator
#endif

#endif //CHARGELAB_OPEN_FIRMWARE_LOGGING_H
