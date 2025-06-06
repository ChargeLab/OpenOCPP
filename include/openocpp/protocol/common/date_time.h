#ifndef CHARGELAB_OPEN_FIRMWARE_DATE_TIME_H
#define CHARGELAB_OPEN_FIRMWARE_DATE_TIME_H

#include "openocpp/model/system_types.h"
#include "openocpp/helpers/json.h"
#include "openocpp/common/logging.h"

#include <string>
#include <utility>

#include <ctime>
#include <stdio.h>
#include <chrono>

#if defined (USE_HH_DATE)
#include <date/date.h>
#endif

namespace chargelab::common {
    class DateTime {
        static constexpr int kMilliSecondSize = 3;
    public:
        DateTime() = default;
        DateTime(const DateTime& other) = default;
        DateTime& operator=(const DateTime& other) = default;

        DateTime(SystemTimeMillis timestamp) : text_(timestampToText(timestamp)), timestamp_(timestamp) {
        }
        DateTime(std::string text) : text_(text), timestamp_(textToTimestamp(text)) {
        }

        [[nodiscard]] std::optional<std::string> getText() const {
            return text_;
        }

        [[nodiscard]] std::optional<SystemTimeMillis> getTimestamp() const {
            return timestamp_;
        }

        static void write_json(json::JsonWriter& writer, DateTime const& value) {
            if (value.text_.has_value()) {
                writer.String(value.text_.value());
            } else {
                writer.Null();
            }
        }

        static bool read_json(json::JsonReader& reader, DateTime& value) {
            auto const token = reader.nextToken();
            if (!token.has_value())
                return false;
            if (!std::holds_alternative<json::StringType>(token.value()))
                return false;

            auto const& text = std::get<json::StringType>(token.value());
            value.text_ = std::string{text.str, text.length};
            value.timestamp_ = textToTimestamp(value.text_.value());
            return true;
        }

        bool operator==(DateTime const& rhs) const {
            return getTimestamp() == rhs.getTimestamp();
        }

        bool operator!=(DateTime const& rhs) const {
            return !operator==(rhs);
        }

        [[nodiscard]] bool isAfter(SystemTimeMillis ts, bool default_value) const {
            if (!timestamp_.has_value())
                return default_value;

            return timestamp_.value() - ts > 0;
        }

        [[nodiscard]] bool isBefore(SystemTimeMillis ts, bool default_value) const {
            if (!timestamp_.has_value())
                return default_value;

            return timestamp_.value() - ts < 0;
        }

#if !defined(USE_HH_DATE)
        static std::optional<std::string> timestampToText(SystemTimeMillis const& timestamp) {
            time_t time = timestamp/1000;
            int millisecond = timestamp%1000;
            if (millisecond < 0) {  // adjust the minus value to be positive, borrow 1 second
                time -= 1;
                millisecond += 1000;
            }

            auto tm = gmtime(&time);
            char buf[100]{};
            sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02dZ", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                    tm->tm_hour, tm->tm_min, tm->tm_sec);
            return std::string(buf);
        }

        static std::optional<SystemTimeMillis> textToTimestamp(std::string const& text) {
            auto text2 = removeExcessiveFractionDigits(text);
            auto timezone_minutes = retrieveTimezoneMinutes(text2);
            if (!timezone_minutes.has_value()) {
                CHARGELAB_LOG_MESSAGE(error) << "wrong time zone format:" << text;
                return std::nullopt;
            }
            // "2022-12-17T14:37:09.894Z"
            auto tm = parseTm(text2);
            if (tm) {
                std::int64_t ret = timegm2(&tm.value());  // timegm() is not supported by ESP32
                ret -= timezone_minutes.value() * 60;
                return static_cast<SystemTimeMillis>(ret * 1000 + retrieveMillisecond(text2));
            }
            CHARGELAB_LOG_MESSAGE(error) << "wrong date time format:" << text;
            return std::nullopt;
        }
#else
        static std::optional<std::string> timestampToText(SystemTimeMillis const& timestamp) {
            date::sys_time<std::chrono::milliseconds> ts {std::chrono::milliseconds(timestamp)};

            CHARGELAB_TRY {
                std::ostringstream out;

                // TODO: Need to confirm this *always* uses UTC timezone
                out << date::format("%FT%TZ", ts);
                if (!out.fail()) {
                    return out.str();
                }
            } CHARGELAB_CATCH {
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected exception converting timestamp to date/time string: " << e.what();
            }

            return std::nullopt;
        }

        static std::optional<SystemTimeMillis> textToTimestamp(std::string const& text) {
            auto text2 = removeExcessiveFractionDigits(text);

            date::sys_time<std::chrono::milliseconds> ts;

            CHARGELAB_TRY {
                std::istringstream in{text2};
                in >> date::parse("%FT%TZ", ts);
                if (!in.fail()) {
                    auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(ts.time_since_epoch()).count();
                    return static_cast<SystemTimeMillis> (millis);
                }
            } CHARGELAB_CATCH {
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected exception converting date/time string to timestamp: " << e.what();
            }

            CHARGELAB_TRY {
                std::istringstream in{text2};
                in >> date::parse("%FT%T%Ez", ts);
                if (!in.fail()) {
                    auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(ts.time_since_epoch()).count();
                    return static_cast<SystemTimeMillis> (millis);
                }
            } CHARGELAB_CATCH {
                CHARGELAB_LOG_MESSAGE(error) << "Unexpected exception converting date/time string to timestamp: " << e.what();
            }

            return std::nullopt;
        }
#endif
    private:
        static std::string removeExcessiveFractionDigits(std::string const& time_text) {
            int pos = time_text.find('.');
            if (pos == -1) return time_text;
            int decimal_digit_count = 0;
            unsigned int i = pos + 1;
            while (i < time_text.length() && decimal_digit_count < kMilliSecondSize) {
                if (!std::isdigit(time_text[i])) break;
                ++decimal_digit_count;
                ++i;
            }
            if (decimal_digit_count == kMilliSecondSize) {
                auto start = i;
                while (i < time_text.length()) {
                    if (!std::isdigit(time_text[i])) break;
                    ++i;
                }
                if (start != i) {
                    return time_text.substr(0, start) + time_text.substr(i);
                }
            }
            return time_text;
        }

        static int retrieveMillisecond(std::string const& timestamp) {
            auto find = timestamp.find('.');
            if (find == std::string::npos) {
                return 0;
            }

            int milliseconds = 0;
            unsigned int multiplier = 100;
            for (unsigned int i = find+1; i < timestamp.length(); ++i) {
                if (!std::isdigit(timestamp[i])) break;
                milliseconds += (timestamp[i] - '0') * multiplier;
                multiplier /= 10;
            }

            return milliseconds;
        }

        // timestamp: e.g. "2022-10-17T14:37:09.894+03:00" https://www.w3.org/TR/NOTE-datetime
        static std::optional<int> retrieveTimezoneMinutes(std::string & timestamp) {
            auto find = timestamp.find('Z');
            if (find != std::string::npos) {
                timestamp = timestamp.substr(0, find); // ignore anything after 'Z'
                return 0;
            }

            bool negative = false;
            find = timestamp.find('+');
            if (find == std::string::npos) {
                find = timestamp.rfind('-');
                negative = true;
            }
            if (find == std::string::npos) return std::nullopt;

            int hour = 0;
            int minute = 0;

            auto find2 = timestamp.find(':', find);
            if (find2 == std::string::npos) {   // only have hours
                if (sscanf(timestamp.c_str() + find + 1, "%2d", &hour) != 1) return std::nullopt;
            } else {
                if (sscanf(timestamp.c_str() + find + 1, "%2d:%2d", &hour, &minute) != 2) return std::nullopt;
            }

            timestamp = timestamp.substr(0, find);  // remove timezone from the timestamp for future process
            return (hour*60 + minute)*(negative ? -1 : 1);
        }

        static std::optional<std::tm> parseTm(std::string const& timestamp) {
            int year, month, day, hour, minute, second;

            if (sscanf(timestamp.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second) != 6) {
                return std::nullopt;
            }
            //
            if (month < 1 || month > 12 || day < 1 || day > getDaysOfMonth(year, month) || hour < 0 || hour >= 24 ||
                minute < 0 || minute >= 60 || second < 0 || second >= 60) { // no leap seconds supported
                return std::nullopt;
            }

            std::tm tm{};

            tm.tm_sec = second;
            tm.tm_min = minute;
            tm.tm_hour = hour;
            tm.tm_mday = day;
            tm.tm_mon = month-1;
            tm.tm_year = year-1900;

            return tm;
        };

        static std::time_t timegm2(std::tm const *tm) {
            if (tm == nullptr) return -1;
            if (tm->tm_mon >= 12 || tm->tm_hour >= 24 || tm->tm_min >= 60 || tm->tm_sec >= 61) {
                return -1;
            }
            if (getDaysOfMonth(tm->tm_year + 1900, tm->tm_mon + 1) == -1) {
                return -1;
            }

            // get the day to the tm date since 1900-01-01 00:00:00 +0000, UTC instead of the Epoch (1970-01-01 00:00:00 +0000, UTC)
            std::int64_t total_days = 0; //
            int year = tm->tm_year + 1900;

            auto const year_since_epch = year - 1970;

            int leap_years = totalLeapYears(year - 1) - totalLeapYears(1970);

            total_days = year_since_epch*365 + leap_years;

            for (int i = 0; i < tm->tm_mon; ++i) {
                total_days += getDaysOfMonth(year, i+1);
            }
            total_days += tm->tm_mday - 1;  // tm_mday is 1-31
            std::int64_t total_hours = total_days*24 + tm->tm_hour;
            std::time_t total_seconds = (total_hours*60 + tm->tm_min)*60 + tm->tm_sec;

            return total_seconds /*+ kSecondsFrom1990*/;
        }

        static bool isLeapYear(unsigned int year) {
            return (year%4 == 0) && (!(year%100 == 0) || (year%400 == 0));
        }
        // month: 1-based
        static int getDaysOfMonth(unsigned int year, int month) {
            switch (month) {
                case 1:
                case 3:
                case 5:
                case 7:
                case 8:
                case 10:
                case 12:
                    return 31;
                case 4:
                case 6:
                case 9:
                case 11:
                    return 30;
                case 2:
                    return isLeapYear(year) ? 29 : 28;
                default:
                    return -1;
            }
        }

        static int totalLeapYears(int year) {
            return year/4 - year/100 + year/400;
        }
    private:
        std::optional<std::string> text_ = std::nullopt;
        std::optional<SystemTimeMillis> timestamp_ = std::nullopt;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_DATE_TIME_H
