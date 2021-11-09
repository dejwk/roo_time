#include "roo_time.h"

#if defined(ESP32) || defined(ESP8266)
#include "esp_attr.h"

extern "C" {
int64_t esp_timer_get_time();
}

inline  static int64_t __uptime() { return esp_timer_get_time(); }

#elif defined(ARDUINO) || defined(ROO_EMULATOR)

#include <Arduino.h>

inline static int64_t __uptime() { return micros(); }

#elif defined(__linux__)
#include <chrono>

inline static int64_t __uptime() {
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(
      now.time_since_epoch()).count();
}
#endif

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

namespace roo_time {

static int64_t last_reading = 0;

// Offset to make sure the time is monotone.
static int64_t offset = 0;

const Uptime IRAM_ATTR Uptime::Now() {
  int64_t now = __uptime() + offset;
  int64_t diff = last_reading - now;
  if (diff > 0) {
    offset += diff;
    now += diff;
  }
  last_reading = now;
  return Uptime(now);
}

namespace {

// Credit:
// https://stackoverflow.com/questions/7960318/math-to-convert-seconds-since-1970-into-date-and-vice-versa

// Returns number of days since civil 1970-01-01.  Negative values indicate
//    days prior to 1970-01-01.
// Preconditions:  y-m-d represents a date in the civil (Gregorian) calendar
//                 m is in [1, 12]
//                 d is in [1, last_day_of_month(y, m)]
//                 y is "approximately" in
//                   [numeric_limits<Int>::min()/366,
//                   numeric_limits<Int>::max()/366]
//                 Exact range of validity is:
//                 [civil_from_days(numeric_limits<Int>::min()),
//                  civil_from_days(numeric_limits<Int>::max()-719468)]
int32_t days_from_civil(int32_t y, uint8_t m, uint8_t d) noexcept {
  y -= m <= 2;
  const int32_t era = (y >= 0 ? y : y - 399) / 400;
  const uint32_t yoe = static_cast<uint16_t>(y - era * 400);  // [0, 399]
  const uint32_t doy =
      (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;          // [0, 365]
  const uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;  // [0, 146096]
  return era * 146097 + static_cast<int32_t>(doe) - 719468;
}

// Returns year/month/day triple in civil calendar
// Preconditions:  z is number of days since 1970-01-01 and is in the range:
//                   [numeric_limits<Int>::min(),
//                   numeric_limits<Int>::max()-719468].
void civil_from_days(int32_t z, int16_t* year, uint8_t* month,
                     uint8_t* day) noexcept {
  z += 719468;
  const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
  const uint32_t doe = static_cast<uint32_t>(z - era * 146097);  // [0, 146096]
  const uint32_t yoe =
      (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;  // [0, 399]
  const int32_t y = static_cast<int32_t>(yoe) + era * 400;
  const uint16_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);  // [0, 365]
  const uint8_t mp = (5 * doy + 2) / 153;                        // [0, 11]
  const uint8_t d = doy - (153 * mp + 2) / 5 + 1;                // [1, 31]
  const uint8_t m = mp + (mp < 10 ? 3 : -9);                     // [1, 12]
  *year = y + (m <= 2);
  *month = m;
  *day = d;
}

// Returns day of week in civil calendar [0, 6] -> [Sun, Sat]
// Preconditions:  z is number of days since 1970-01-01 and is in the range:
//                   [numeric_limits<Int>::min(), numeric_limits<Int>::max()-4].
constexpr DayOfWeek weekday_from_days(int32_t z) noexcept {
  return static_cast<DayOfWeek>(z >= -4 ? (z + 4) % 7 : (z + 5) % 7 + 6);
}

// Returns: true if y is a leap year in the civil calendar, else false
constexpr bool is_leap(int32_t y) noexcept {
  return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

// Preconditions:  y-m-d represents a date in the civil (Gregorian) calendar
//                 m is in [1, 12]
//                 d is in [1, last_day_of_month(y, m)]
uint16_t day_of_year(int16_t y, uint8_t m, uint8_t d) {
  constexpr uint16_t days_to_month[12] = {0,   31,  59,  90,  120, 151,
                                          181, 212, 243, 273, 304, 334};
  uint16_t result = days_to_month[m - 1] + d;
  if (m > 2 && is_leap(y)) result++;
  return result;
}

// Credit:
// https://stackoverflow.com/questions/1082917/mod-of-negative-number-is-melting-my-brain/1082938#1082938
// Assumes n > 0.
template <typename Int>
constexpr Int floor_mod(Int k, Int n) {
  return ((k %= n) < 0) ? k + n : k;
}

}  // namespace

DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, TimeZone tz)
    : DateTime(year, month, day, 0, 0, 0, 0, tz) {}

DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour,
                   uint8_t minute, uint8_t second, uint32_t micros, TimeZone tz)
    : tz_(tz),
      year_(year),
      month_(month),
      day_(day),
      hour_(hour),
      minute_(minute),
      second_(second),
      micros_(micros) {
  int64_t t = days_from_civil(year, month, day);
  day_of_week_ = weekday_from_days(t);
  t = ((((t * 24) + hour) * 60 + minute) * 60 + second) * 1000000 + micros;
  day_of_year_ = day_of_year(year, month, day);
  walltime_ = WallTime(Micros(t) - tz.offset());
}

DateTime::DateTime(WallTime wall_time, TimeZone tz)
    : walltime_(wall_time), tz_(tz) {
  Interval sinceEpochTz = wall_time.sinceEpoch() + tz.offset();
  int32_t unix_days = sinceEpochTz.inHours() / 24;
  civil_from_days(unix_days, &year_, &month_, &day_);
  day_of_year_ = day_of_year(year_, month_, day_);
  day_of_week_ = weekday_from_days(unix_days);
  uint64_t since_midnight = floor_mod<int64_t>(sinceEpochTz.inMicros(),
                                               (uint64_t)1000000 * 3600 * 24);
  micros_ = since_midnight % 1000000L;
  since_midnight /= 1000000L;
  second_ = since_midnight % 60;
  since_midnight /= 60;
  minute_ = since_midnight % 60;
  since_midnight /= 60;
  hour_ = since_midnight;
}

}  // namespace roo_time
