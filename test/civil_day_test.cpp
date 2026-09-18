#include "roo_time/civil_day.h"

#include <type_traits>

#include "gtest/gtest.h"
#include "roo_time/timezone.h"

namespace roo_time {
namespace {

static_assert(sizeof(CivilDay) == 4, "CivilDay must remain four bytes");
static_assert(std::is_trivially_copyable<CivilDay>::value, "Value type");
static_assert(!CivilDay().isValid(), "Default is invalid");
static_assert(CivilDay() == CivilDay::Invalid(), "Invalid equality");

// Verifies invalid components are rejected before narrowing or normalization.
TEST(CivilDay, CheckedConstruction) {
  const int32_t invalid[][3] = {{0, 1, 1},
                                {10000, 1, 1},
                                {-1, 1, 1},
                                {65537, 1, 1},
                                {2024, 0, 1},
                                {2024, 13, 1},
                                {2024, 257, 1},
                                {2024, -1, 1},
                                {2024, 1, 0},
                                {2024, 1, 32},
                                {2024, 1, 257},
                                {2024, 1, -1},
                                {2023, 2, 29},
                                {1900, 2, 29},
                                {2100, 2, 29},
                                {2024, 4, 31},
                                {INT32_MIN, 1, 1},
                                {INT32_MAX, 1, 1},
                                {2024, INT32_MAX, 1},
                                {2024, 1, INT32_MAX}};
  for (const auto& fields : invalid) {
    EXPECT_EQ(CivilDay::Invalid(),
              CivilDay::FromYmd(fields[0], fields[1], fields[2]));
  }
  EXPECT_TRUE(CivilDay::FromYmd(2000, 2, 29).isValid());
  EXPECT_FALSE(CivilDay().isValid());
  EXPECT_NE(CivilDay(), CivilDay::FromYmd(1970, 1, 1));
}

// Verifies Gregorian conversion and weekday continuity across a full 400-year
// cycle, including leap centuries, using independently advanced calendar
// fields.
TEST(CivilDay, GregorianCycle) {
  CivilDay date = CivilDay::FromYmd(1800, 1, 1);
  int weekday = kWednesday;
  int count = 0;
  for (int year = 1800; year < 2200; ++year) {
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    EXPECT_EQ(leap, IsLeapYear(year));
    for (int month = 1; month <= 12; ++month) {
      const int lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
      const int length = lengths[month - 1] + (month == 2 && leap);
      EXPECT_EQ(length, DaysInMonth(year, static_cast<Month>(month)));
      for (int day = 1; day <= length; ++day) {
        ASSERT_EQ(year, date.year());
        ASSERT_EQ(month, date.month());
        ASSERT_EQ(day, date.day());
        ASSERT_EQ(weekday, date.dayOfWeek());
        ASSERT_EQ(CivilDay::FromYmd(year, month, day), date);
        date = date.addDays(1);
        weekday = (weekday + 1) % 7;
        ++count;
      }
    }
  }
  EXPECT_EQ(146097, count);
  EXPECT_EQ(CivilDay::FromYmd(2200, 1, 1), date);
}

// Verifies supported endpoints, negative epoch weekdays, and checked
// arithmetic.
TEST(CivilDay, BoundariesAndArithmetic) {
  const CivilDay first = CivilDay::FromYmd(1, 1, 1);
  const CivilDay last = CivilDay::FromYmd(9999, 12, 31);
  EXPECT_EQ(1, first.year());
  EXPECT_EQ(kJanuary, first.month());
  EXPECT_EQ(1, first.day());
  EXPECT_EQ(kMonday, first.dayOfWeek());
  EXPECT_EQ(9999, last.year());
  EXPECT_EQ(kDecember, last.month());
  EXPECT_EQ(31, last.day());
  EXPECT_EQ(kFriday, last.dayOfWeek());
  EXPECT_EQ(last, first.addDays(3652058));
  EXPECT_EQ(first, last.addDays(-3652058));
  for (CivilDay date : {first, last, CivilDay::FromYmd(1970, 1, 1)}) {
    EXPECT_EQ(date, date.addDays(0));
    EXPECT_FALSE(date.addDays(INT32_MIN).isValid());
    EXPECT_FALSE(date.addDays(INT32_MAX).isValid());
  }
  EXPECT_FALSE(first.addDays(-1).isValid());
  EXPECT_FALSE(last.addDays(1).isValid());
  EXPECT_FALSE(CivilDay().addDays(INT32_MAX).isValid());
  const CivilDay epoch = CivilDay::FromYmd(1970, 1, 1);
  EXPECT_EQ(kThursday, epoch.dayOfWeek());
  EXPECT_EQ(kWednesday, epoch.addDays(-1).dayOfWeek());
  EXPECT_EQ(kSaturday, epoch.addDays(-5).dayOfWeek());
  EXPECT_EQ(CivilDay::FromYmd(2000, 2, 29),
            CivilDay::FromYmd(2000, 3, 1).addDays(-1));
  EXPECT_EQ(CivilDay::FromYmd(1900, 2, 28),
            CivilDay::FromYmd(1900, 3, 1).addDays(-1));
  EXPECT_TRUE(first < last);
  EXPECT_TRUE(last > first);
  EXPECT_TRUE(first <= first);
  EXPECT_TRUE(last >= last);
  EXPECT_FALSE(last < first);
  EXPECT_FALSE(first > last);
}

// Verifies extraction keeps the local date across UTC midnight and DST changes.
TEST(CivilDay, LocalDateTimeExtraction) {
  const WallTime instant = DateTime(2024, 1, 1, timezone::UTC).wallTime();
  EXPECT_EQ(CivilDay::FromYmd(2023, 12, 31),
            DateTime(instant, UtcOffset(Hours(-1))).civilDay());
  EXPECT_EQ(CivilDay::FromYmd(2024, 1, 1),
            DateTime(instant, UtcOffset(Hours(1))).civilDay());
  RecurringTimeZone zone(UtcOffset(Hours(1)), rules::EuSummerTime());
  const DateTime local(2024, 3, 30, 23, 30, 0, 0, UtcOffset(Hours(1)));
  const CivilDay date = ToLocal(local.wallTime(), zone).civilDay();
  EXPECT_EQ(CivilDay::FromYmd(2024, 3, 31), date.addDays(1));
  EXPECT_EQ(CivilDay::FromYmd(2024, 4, 1),
            ToLocal(local.wallTime() + Hours(24), zone).civilDay());
}

}  // namespace
}  // namespace roo_time
