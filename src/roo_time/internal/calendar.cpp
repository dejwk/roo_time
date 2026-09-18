#include "roo_time/internal/calendar.h"

namespace roo_time {
namespace internal {

// Credit:
// https://stackoverflow.com/questions/7960318/math-to-convert-seconds-since-1970-into-date-and-vice-versa

// Converts a valid Gregorian date to days since 1970-01-01. Moving January
// and February to the preceding year makes each 400-year era uniform.
int32_t DaysFromCivil(int32_t y, uint8_t m, uint8_t d) noexcept {
  y -= m <= 2;
  const int32_t era = (y >= 0 ? y : y - 399) / 400;
  const uint32_t yoe = static_cast<uint16_t>(y - era * 400);  // [0, 399]
  const uint32_t doy =
      (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;          // [0, 365]
  const uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;  // [0, 146096]
  return era * 146097 + static_cast<int32_t>(doe) - 719468;
}

// Inverts the 400-year-era decomposition for a supported epoch day.
void CivilFromDays(int32_t z, int16_t* year, uint8_t* month,
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

// Handles negative epoch days without relying on negative modulo wrapping.
DayOfWeek WeekdayFromDays(int32_t z) noexcept {
  return static_cast<DayOfWeek>(z >= -4 ? (z + 4) % 7 : (z + 5) % 7 + 6);
}

// Preconditions:  y-m-d represents a date in the civil (Gregorian) calendar
//                 m is in [1, 12]
//                 d is in [1, last_day_of_month(y, m)]
uint16_t DayOfYear(int16_t y, uint8_t m, uint8_t d) {
  constexpr uint16_t days_to_month[12] = {0,   31,  59,  90,  120, 151,
                                          181, 212, 243, 273, 304, 334};
  uint16_t result = days_to_month[m - 1] + d;
  if (m > 2 && IsLeapYear(y)) result++;
  return result;
}

}  // namespace internal
}  // namespace roo_time
