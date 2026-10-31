#include "roo_time/wall_time.h"

#include "roo_time/internal/calendar.h"

namespace roo_time {
namespace {

// Credit:
// https://stackoverflow.com/questions/1082917/mod-of-negative-number-is-melting-my-brain/1082938#1082938
// Assumes n > 0.
template <typename Int>
constexpr Int FloorMod(Int k, Int n) {
  return ((k %= n) < 0) ? k + n : k;
}

}  // namespace

DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, UtcOffset tz)
    : DateTime(year, month, day, 0, 0, 0, 0, tz) {}

DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour,
                   uint8_t minute, uint8_t second, uint32_t micros,
                   UtcOffset tz)
    : offset_(tz),
      year_(year),
      month_(month),
      day_(day),
      hour_(hour),
      minute_(minute),
      second_(second),
      micros_(micros) {
  int64_t t = internal::DaysFromCivil(year, month, day);
  day_of_week_ = internal::WeekdayFromDays(t);
  t = ((((t * 24) + hour) * 60 + minute) * 60 + second) * 1000000 + micros;
  day_of_year_ = internal::DayOfYear(year, month, day);
  walltime_ = WallTime::SinceEpoch(Micros(t) - offset_.asDuration());
}

bool DateTime::TryFromComponents(int32_t year, int32_t month, int32_t day,
                                 int32_t hour, int32_t minute, int32_t second,
                                 int32_t micros, UtcOffset offset,
                                 DateTime& result) {
  if (!CivilDay::FromYmd(year, month, day).isValid() || hour < 0 || hour > 23 ||
      minute < 0 || minute > 59 || second < 0 || second > 59 || micros < 0 ||
      micros > 999999) {
    return false;
  }
  result = DateTime(year, month, day, hour, minute, second, micros, offset);
  return true;
}

bool DateTime::TryFromComponents(CivilDay date, TimeOfDay time,
                                 UtcOffset offset, DateTime& result) {
  if (!date.isValid() || !time.isValid()) return false;
  return TryFromComponents(date.year(), date.month(), date.day(), time.hour(),
                           time.minute(), time.second(), time.micros(), offset,
                           result);
}

DateTime::DateTime(WallTime wall_time, UtcOffset tz)
    : walltime_(wall_time), offset_(tz) {
  assert(wall_time.isSet());
  Duration sinceEpochTz = wall_time.sinceEpoch() + offset_.asDuration();
  constexpr int64_t kMicrosPerDay = 86400000000LL;
  const int64_t micros = sinceEpochTz.inMicros();
  int32_t unix_days = micros / kMicrosPerDay;
  if (micros % kMicrosPerDay < 0) --unix_days;
  internal::CivilFromDays(unix_days, &year_, &month_, &day_);
  day_of_year_ = internal::DayOfYear(year_, month_, day_);
  day_of_week_ = internal::WeekdayFromDays(unix_days);
  uint64_t since_midnight =
      FloorMod<int64_t>(sinceEpochTz.inMicros(), (uint64_t)1000000 * 3600 * 24);
  micros_ = since_midnight % 1000000L;
  since_midnight /= 1000000L;
  second_ = since_midnight % 60;
  since_midnight /= 60;
  minute_ = since_midnight % 60;
  since_midnight /= 60;
  hour_ = since_midnight;
}

}  // namespace roo_time
