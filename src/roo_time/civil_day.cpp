#include "roo_time/civil_day.h"

#include "roo_time/internal/calendar.h"

namespace roo_time {

bool IsLeapYear(int32_t year) {
  assert(year >= 1 && year <= 9999);
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

uint8_t DaysInMonth(int32_t year, Month month) {
  assert(year >= 1 && year <= 9999);
  assert(month >= kJanuary && month <= kDecember);
  static constexpr uint8_t days[] = {31, 28, 31, 30, 31, 30,
                                     31, 31, 30, 31, 30, 31};
  return days[month - 1] + (month == kFebruary && IsLeapYear(year));
}

CivilDay CivilDay::FromYmd(int32_t year, int32_t month, int32_t day) {
  if (year < 1 || year > 9999 || month < 1 || month > 12 || day < 1 ||
      day > DaysInMonth(year, static_cast<Month>(month)))
    return Invalid();
  return CivilDay(internal::DaysFromCivil(year, month, day));
}

int16_t CivilDay::year() const {
  assert(isValid());
  int16_t year;
  uint8_t month, day;
  internal::CivilFromDays(days_since_epoch_, &year, &month, &day);
  return year;
}

Month CivilDay::month() const {
  assert(isValid());
  int16_t year;
  uint8_t month, day;
  internal::CivilFromDays(days_since_epoch_, &year, &month, &day);
  return static_cast<Month>(month);
}

uint8_t CivilDay::day() const {
  assert(isValid());
  int16_t year;
  uint8_t month, day;
  internal::CivilFromDays(days_since_epoch_, &year, &month, &day);
  return day;
}

DayOfWeek CivilDay::dayOfWeek() const {
  assert(isValid());
  return internal::WeekdayFromDays(days_since_epoch_);
}

CivilDay CivilDay::addDays(int32_t days) const {
  if (!isValid()) return Invalid();
  const int64_t result = static_cast<int64_t>(days_since_epoch_) + days;
  if (result < internal::DaysFromCivil(1, 1, 1) ||
      result > internal::DaysFromCivil(9999, 12, 31))
    return Invalid();
  return CivilDay(static_cast<int32_t>(result));
}

}  // namespace roo_time
