#ifndef CHARGELAB_OPEN_FIRMWARE_CHRONO_H
#define CHARGELAB_OPEN_FIRMWARE_CHRONO_H

#include "openocpp/model/system_types.h"
#include "openocpp/common/logging.h"

#include <optional>
#include <string>
#include <sstream>
#include "openocpp/protocol/ocpp1_6/types/date_time.h"

namespace chargelab::chrono {
    namespace detail {
        inline bool isLeapYear(unsigned int year) {
            return (year%4 == 0) && (!(year%100 == 0) || (year%400 == 0));
        }
        // month: 1-based
        inline int getDaysOfMonth(unsigned int year, int month) {
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

        inline int totalLeapYears(int year) {
            return year/4 - year/100 + year/400;
        }
    }

    inline std::optional<std::string> ToString(SystemTimeMillis const& timestamp) {
        return ocpp1_6::DateTime::timestampToText(timestamp);
    }

    inline std::optional<SystemTimeMillis> FromString(std::string const& text) {
        return ocpp1_6::DateTime::textToTimestamp(text);
    }

    inline std::time_t timegm2(std::tm const *tm) {
        if (tm == nullptr) return -1;
        if (tm->tm_mon >= 12 || tm->tm_hour >= 24 || tm->tm_min >= 60 || tm->tm_sec >= 61) {
            return -1;
        }
        if (detail::getDaysOfMonth(tm->tm_year + 1900, tm->tm_mon + 1) == -1) {
            return -1;
        }

        // get the day to the tm date since 1900-01-01 00:00:00 +0000, UTC instead of the Epoch (1970-01-01 00:00:00 +0000, UTC)
        std::int64_t total_days = 0; //
        int year = tm->tm_year + 1900;

        auto const year_since_epoch = year - 1970;
        int leap_years = detail::totalLeapYears(year - 1) - detail::totalLeapYears(1970);

        total_days = year_since_epoch * 365 + leap_years;

        for (int i = 0; i < tm->tm_mon; ++i) {
            total_days += detail::getDaysOfMonth(year, i+1);
        }
        total_days += tm->tm_mday - 1;  // tm_mday is 1-31
        std::int64_t total_hours = total_days*24 + tm->tm_hour;
        std::time_t total_seconds = (total_hours*60 + tm->tm_min)*60 + tm->tm_sec;

        return total_seconds /*+ kSecondsFrom1990*/;
    }
}

#endif //CHARGELAB_OPEN_FIRMWARE_CHRONO_H
