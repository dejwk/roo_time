#pragma once

#include "roo_time/civil_day.h"

namespace roo_time {
namespace internal {

// Calendar conversion helpers require valid dates in years 1–9999.
int32_t DaysFromCivil(int32_t year, uint8_t month, uint8_t day) noexcept;

// Requires an epoch day corresponding to a date in years 1–9999.
void CivilFromDays(int32_t days, int16_t* year, uint8_t* month,
                   uint8_t* day) noexcept;

// Returns weekday for a supported epoch day.
DayOfWeek WeekdayFromDays(int32_t days) noexcept;

// Returns one-based day of year for a valid date.
uint16_t DayOfYear(int16_t year, uint8_t month, uint8_t day);

}  // namespace internal
}  // namespace roo_time
