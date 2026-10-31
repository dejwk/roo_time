#include <type_traits>

#include "gtest/gtest.h"
#include "roo_time.h"

namespace roo_time {
namespace {

static_assert(sizeof(TimeOfDay) == sizeof(int64_t), "Eight-byte value");
static_assert(std::is_trivially_copyable<TimeOfDay>::value, "Value type");
static_assert(!TimeOfDay().isValid(), "Default invalid");
static_assert(TimeOfDay::Midnight().isValid(), "Midnight valid");
static_assert(TimeOfDay::FromHms(6, 30).sinceMidnight() == Minutes(390),
              "Constexpr construction and accessors");
static_assert(!std::is_convertible<Duration, TimeOfDay>::value,
              "Checked explicit factory required");

// Verifies exact fields, boundary values, and inverse duration conversion.
TEST(TimeOfDay, CheckedConstructionAndComponents) {
  for (int hour = 0; hour < 24; ++hour) {
    for (int minute = 0; minute < 60; ++minute) {
      for (int second : {0, 1, 59}) {
        for (int micros : {0, 1, 500000, 999999}) {
          const TimeOfDay time =
              TimeOfDay::FromHms(hour, minute, second, micros);
          ASSERT_TRUE(time.isValid());
          EXPECT_EQ(hour, time.hour());
          EXPECT_EQ(minute, time.minute());
          EXPECT_EQ(second, time.second());
          EXPECT_EQ(micros, time.micros());
          EXPECT_EQ(time, TimeOfDay::FromSinceMidnight(time.sinceMidnight()));
        }
      }
    }
  }
  EXPECT_EQ(TimeOfDay::Midnight(), TimeOfDay::FromHms(0, 0));
  EXPECT_EQ(TimeOfDay::FromHms(23, 59, 59, 999999),
            TimeOfDay::FromSinceMidnight(Hours(24) - Micros(1)));
}

// Verifies invalid fields are rejected before narrowing or normalization.
TEST(TimeOfDay, InvalidInputs) {
  const int32_t above[] = {24, 60, 60, 1000000};
  for (int field = 0; field < 4; ++field) {
    for (int32_t bad : {-1, INT32_MIN, INT32_MAX, above[field]}) {
      int32_t values[] = {0, 0, 0, 0};
      values[field] = bad;
      EXPECT_FALSE(
          TimeOfDay::FromHms(values[0], values[1], values[2], values[3])
              .isValid());
    }
  }
  for (Duration invalid :
       {Duration::Min(), Micros(-1), Hours(24), Duration::Max()}) {
    EXPECT_EQ(TimeOfDay::Invalid(), TimeOfDay::FromSinceMidnight(invalid));
  }
  EXPECT_EQ(TimeOfDay(), TimeOfDay::Invalid());
  EXPECT_NE(TimeOfDay(), TimeOfDay::Midnight());
}

// Verifies clock order is linear within a day, without midnight wrapping.
TEST(TimeOfDay, Comparisons) {
  const TimeOfDay first = TimeOfDay::Midnight();
  const TimeOfDay last = TimeOfDay::FromHms(23, 59, 59, 999999);
  EXPECT_LT(first, last);
  EXPECT_GT(last, first);
  EXPECT_LE(first, first);
  EXPECT_GE(last, last);
  EXPECT_FALSE(last < first);
  EXPECT_FALSE(first > last);
  EXPECT_FALSE(first >= last);
  EXPECT_FALSE(last <= first);
  EXPECT_NE(first, last);
}

// Verifies fixed-offset composition and extraction keep local microseconds.
TEST(TimeOfDay, DateTimeIntegration) {
  const CivilDay date = CivilDay::FromYmd(2026, 11, 1);
  const TimeOfDay time = TimeOfDay::FromHms(0, 30, 59, 999999);
  DateTime result;
  ASSERT_TRUE(
      DateTime::TryFromComponents(date, time, UtcOffset(Hours(2)), result));
  EXPECT_EQ(date, result.civilDay());
  EXPECT_EQ(time, result.timeOfDay());
  EXPECT_EQ(TimeOfDay::FromHms(22, 30, 59, 999999),
            DateTime(result.wallTime(), timezone::UTC).timeOfDay());
  EXPECT_EQ(CivilDay::FromYmd(2026, 10, 31),
            DateTime(result.wallTime(), timezone::UTC).civilDay());
  const DateTime original = result;
  EXPECT_FALSE(DateTime::TryFromComponents(CivilDay::Invalid(), time,
                                           timezone::UTC, result));
  EXPECT_EQ(original, result);
  EXPECT_FALSE(DateTime::TryFromComponents(date, TimeOfDay::Invalid(),
                                           timezone::UTC, result));
  EXPECT_EQ(original, result);
}

#ifndef NDEBUG
// Verifies invalid values cannot be read or ordered as ordinary clock times.
TEST(TimeOfDayDeathTest, InvalidAccessAndOrdering) {
  const TimeOfDay invalid;
  EXPECT_DEATH({ (void)invalid.hour(); }, "");
  EXPECT_DEATH({ (void)invalid.minute(); }, "");
  EXPECT_DEATH({ (void)invalid.second(); }, "");
  EXPECT_DEATH({ (void)invalid.micros(); }, "");
  EXPECT_DEATH({ (void)invalid.sinceMidnight(); }, "");
  EXPECT_DEATH({ (void)(invalid < TimeOfDay::Midnight()); }, "");
  EXPECT_DEATH({ (void)(TimeOfDay::Midnight() < invalid); }, "");
}
#endif

}  // namespace
}  // namespace roo_time
