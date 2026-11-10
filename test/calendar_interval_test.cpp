#include "roo_time/calendar_interval.h"

#include <vector>

#include "gtest/gtest.h"
#include "test/synthetic_calendar_zone.h"

namespace roo_time {
namespace {

WallTime Utc(int y, int m, int d, int h = 0, int min = 0) {
  return DateTime(y, m, d, h, min, 0, 0, timezone::UTC).wallTime();
}

bool Collect(void* context, const CalendarBoundary& boundary) {
  static_cast<std::vector<CalendarBoundary>*>(context)->push_back(boundary);
  return true;
}

bool Stop(void*, const CalendarBoundary&) { return false; }

// Checks exact sequences and the UTC ordering, containment and round trip.
void ExpectSequence(TimeRange range, const TimeZone& zone,
                    CalendarInterval interval,
                    const std::vector<WallTime>& expected) {
  std::vector<CalendarBoundary> actual;
  uint32_t budget = 4096;
  ASSERT_EQ(CalendarStatus::kOk,
            EnumerateCalendarBoundaries(range, zone, interval, budget, Collect,
                                        &actual));
  ASSERT_EQ(expected.size(), actual.size());
  for (size_t i = 0; i < actual.size(); ++i) {
    EXPECT_EQ(expected[i], actual[i].instant);
    EXPECT_GE(actual[i].instant, range.begin);
    EXPECT_LT(actual[i].instant, range.end);
    EXPECT_EQ(zone.resolveOffset(actual[i].instant), actual[i].offset);
    const DateTime local(actual[i].instant, actual[i].offset);
    const LocalTimeResolution reverse =
        zone.resolveLocal(local.civilDay(), local.timeOfDay());
    EXPECT_TRUE(reverse.first == actual[i].instant ||
                reverse.second == actual[i].instant);
    if (i != 0) {
      EXPECT_LT(actual[i - 1].instant, actual[i].instant);
    }
  }
}

// Verifies each accepted field divisor and rejects zero, parent-sized and
// non-dividing steps, unknown units, and invalid relevant anchors/weekdays.
TEST(CalendarInterval, Validation) {
  for (CalendarUnit unit :
       {CalendarUnit::kMillisecond, CalendarUnit::kSecond,
        CalendarUnit::kMinute, CalendarUnit::kHour, CalendarUnit::kMonth}) {
    const int parent = unit == CalendarUnit::kMillisecond ? 1000
                       : unit == CalendarUnit::kHour      ? 24
                       : unit == CalendarUnit::kMonth     ? 12
                                                          : 60;
    for (int step = 0; step <= parent; ++step) {
      EXPECT_EQ(
          step > 0 && step < parent && parent % step == 0,
          (CalendarInterval{unit, static_cast<uint16_t>(step)}.isValid()));
    }
  }
  EXPECT_TRUE(CalendarInterval::Days(65535).isValid());
  EXPECT_TRUE(CalendarInterval::Weeks(kSunday, 65535).isValid());
  EXPECT_FALSE(CalendarInterval::Days(0).isValid());
  EXPECT_FALSE(CalendarInterval::Days(1, CivilDay()).isValid());
  EXPECT_FALSE(CalendarInterval::Weeks(static_cast<DayOfWeek>(7)).isValid());
  EXPECT_TRUE((CalendarInterval{CalendarUnit::kYear, 9999}.isValid()));
  EXPECT_FALSE((CalendarInterval{CalendarUnit::kYear, 10000}.isValid()));
  EXPECT_FALSE((CalendarInterval{static_cast<CalendarUnit>(255), 1}.isValid()));
  EXPECT_FALSE((CalendarInterval{CalendarUnit::kDay, 1}.isValid()));
}

// Verifies all sub-day units align in a fractional-hour zone, with floor
// division across the epoch and inclusive begin/exclusive end at microseconds.
TEST(CalendarInterval, SubDayAlignmentAndPreEpoch) {
  FixedTimeZone utc(timezone::UTC);
  const WallTime epoch = WallTime::Epoch();
  ExpectSequence({epoch - Micros(1501), epoch + Micros(2000)}, utc,
                 {CalendarUnit::kMillisecond, 1},
                 {epoch - Micros(1000), epoch, epoch + Micros(1000)});
  ExpectSequence({epoch - Seconds(31), epoch + Seconds(30)}, utc,
                 {CalendarUnit::kSecond, 30}, {epoch - Seconds(30), epoch});
  ExpectSequence({epoch - Minutes(31), epoch + Minutes(30)}, utc,
                 {CalendarUnit::kMinute, 30}, {epoch - Minutes(30), epoch});
  FixedTimeZone nepal(UtcOffset(Minutes(345)));
  ExpectSequence({Utc(2026, 1, 1), Utc(2026, 1, 2)}, nepal,
                 {CalendarUnit::kHour, 6},
                 {Utc(2026, 1, 1, 0, 15), Utc(2026, 1, 1, 6, 15),
                  Utc(2026, 1, 1, 12, 15), Utc(2026, 1, 1, 18, 15)});
}

// Verifies leap days, long anchored steps, and week normalization before the
// epoch and at year 1, without scanning from or wrapping around the anchor.
TEST(CalendarInterval, AnchoredDaysAndWeeks) {
  FixedTimeZone utc(timezone::UTC);
  ExpectSequence({Utc(2024, 2, 28, 12), Utc(2024, 3, 2)}, utc,
                 CalendarInterval::Days(), {Utc(2024, 2, 29), Utc(2024, 3, 1)});
  ExpectSequence(
      {Utc(1969, 12, 26), Utc(1970, 1, 5)}, utc, CalendarInterval::Days(3),
      {Utc(1969, 12, 26), Utc(1969, 12, 29), Utc(1970, 1, 1), Utc(1970, 1, 4)});
  ExpectSequence({Utc(1969, 12, 20), Utc(1970, 1, 20)}, utc,
                 CalendarInterval::Weeks(kSunday, 2),
                 {Utc(1969, 12, 21), Utc(1970, 1, 4), Utc(1970, 1, 18)});
  ExpectSequence(
      {Utc(2026, 1, 1), Utc(2026, 1, 14)}, utc,
      CalendarInterval::Weeks(kWednesday, 1, CivilDay::FromYmd(2026, 1, 9)),
      {Utc(2026, 1, 7)});
  ExpectSequence(
      {Utc(1, 1, 1), Utc(1, 1, 15)}, utc,
      CalendarInterval::Weeks(kSunday, 1, CivilDay::FromYmd(1, 1, 1)),
      {Utc(1, 1, 7), Utc(1, 1, 14)});
  ExpectSequence({Utc(9999, 12, 30), Utc(9999, 12, 31) + Hours(24)}, utc,
                 CalendarInterval::Days(65535, CivilDay::FromYmd(9999, 12, 31)),
                 {Utc(9999, 12, 31)});
}

// Verifies unequal month lengths, year alignment and arithmetic sentinels at
// both calendar edges, including steps whose next year exceeds 9999.
TEST(CalendarInterval, MonthsYearsAndCalendarEdges) {
  FixedTimeZone utc(timezone::UTC);
  ExpectSequence({Utc(2024, 1, 31), Utc(2024, 5, 1)}, utc,
                 {CalendarUnit::kMonth, 1},
                 {Utc(2024, 2, 1), Utc(2024, 3, 1), Utc(2024, 4, 1)});
  ExpectSequence({Utc(2025, 11, 15), Utc(2026, 10, 1)}, utc,
                 {CalendarUnit::kMonth, 3},
                 {Utc(2026, 1, 1), Utc(2026, 4, 1), Utc(2026, 7, 1)});
  ExpectSequence({Utc(1999, 12, 31), Utc(2041, 1, 1)}, utc,
                 {CalendarUnit::kYear, 20},
                 {Utc(2000, 1, 1), Utc(2020, 1, 1), Utc(2040, 1, 1)});
  ExpectSequence({Utc(1, 1, 1), Utc(4, 1, 1)}, utc, {CalendarUnit::kYear, 2},
                 {Utc(2, 1, 1)});
  const WallTime end = Utc(9999, 12, 31) + Hours(24);
  ExpectSequence({Utc(9999, 1, 1), end}, utc, {CalendarUnit::kYear, 9999},
                 {Utc(9999, 1, 1)});
  ExpectSequence({Utc(9999, 12, 31), end}, utc, {CalendarUnit::kMonth, 1}, {});
  ExpectSequence({end - Micros(2000), end}, utc,
                 {CalendarUnit::kMillisecond, 1},
                 {end - Micros(2000), end - Micros(1000)});
}

// Verifies exact UTC sequences through spring gaps and autumn folds; a later
// occurrence stays ambiguous when the earlier occurrence is outside the range.
TEST(CalendarInterval, HourlyGapsFoldsAndViewportOccurrence) {
  RecurringTimeZone eu(UtcOffset(Hours(1)), rules::EuSummerTime());
  ExpectSequence({Utc(2026, 3, 29), Utc(2026, 3, 29, 4)}, eu,
                 {CalendarUnit::kHour, 1},
                 {Utc(2026, 3, 29), Utc(2026, 3, 29, 1), Utc(2026, 3, 29, 2),
                  Utc(2026, 3, 29, 3)});
  std::vector<CalendarBoundary> boundaries;
  uint32_t budget = 50;
  ASSERT_EQ(CalendarStatus::kOk,
            EnumerateCalendarBoundaries(
                {Utc(2026, 10, 25), Utc(2026, 10, 25, 3)}, eu,
                {CalendarUnit::kHour, 1}, budget, Collect, &boundaries));
  ASSERT_EQ(3u, boundaries.size());
  EXPECT_EQ(Utc(2026, 10, 25), boundaries[0].instant);
  EXPECT_EQ(LocalOccurrence::kEarlier, boundaries[0].occurrence);
  EXPECT_EQ(Utc(2026, 10, 25, 1), boundaries[1].instant);
  EXPECT_EQ(LocalOccurrence::kLater, boundaries[1].occurrence);
  EXPECT_EQ(LocalOccurrence::kUnique, boundaries[2].occurrence);
  boundaries.clear();
  ASSERT_EQ(CalendarStatus::kOk,
            EnumerateCalendarBoundaries(
                {Utc(2026, 10, 25, 0, 30), Utc(2026, 10, 25, 2)}, eu,
                {CalendarUnit::kHour, 1}, budget, Collect, &boundaries));
  ASSERT_EQ(1u, boundaries.size());
  EXPECT_EQ(LocalOccurrence::kLater, boundaries[0].occurrence);
  RecurringTimeZone lord_howe(UtcOffset(Hours(11)), rules::LordHoweDst());
  ExpectSequence({Utc(2026, 4, 4, 14), Utc(2026, 4, 4, 16)}, lord_howe,
                 {CalendarUnit::kMinute, 30},
                 {Utc(2026, 4, 4, 14), Utc(2026, 4, 4, 14, 30),
                  Utc(2026, 4, 4, 15), Utc(2026, 4, 4, 15, 30)});
}

// Verifies missing/repeated midnights and a complete skipped date have their
// actual UTC boundaries, without manufacturing replacement midnight readings.
TEST(CalendarInterval, MidnightChangesAndSkippedDate) {
  using testing::SyntheticCalendarZone;
  const TimeRange supported{Utc(2000, 1, 1), Utc(2030, 1, 1)};
  const SyntheticCalendarZone::Change spring[] = {
      {Utc(2026, 3, 29), UtcOffset(Hours(1))}};
  SyntheticCalendarZone gap(supported, timezone::UTC, spring, 1);
  ExpectSequence(
      {Utc(2026, 3, 28), Utc(2026, 3, 31)}, gap, CalendarInterval::Days(),
      {Utc(2026, 3, 28), Utc(2026, 3, 29, 23), Utc(2026, 3, 30, 23)});
  const SyntheticCalendarZone::Change autumn[] = {
      {Utc(2026, 10, 25), timezone::UTC}};
  SyntheticCalendarZone fold(supported, UtcOffset(Hours(1)), autumn, 1);
  ExpectSequence({Utc(2026, 10, 24, 22), Utc(2026, 10, 26)}, fold,
                 CalendarInterval::Days(),
                 {Utc(2026, 10, 24, 23), Utc(2026, 10, 25)});
  const SyntheticCalendarZone::Change dateline[] = {
      {Utc(2011, 12, 30, 10), UtcOffset(Hours(14))}};
  SyntheticCalendarZone skip(supported, UtcOffset(Hours(-10)), dateline, 1);
  ExpectSequence(
      {Utc(2011, 12, 29), Utc(2012, 1, 1)}, skip, CalendarInterval::Days(),
      {Utc(2011, 12, 29, 10), Utc(2011, 12, 30, 10), Utc(2011, 12, 31, 10)});
}

// Verifies shared budgets, exhaustion before each kind of work, early visitor
// stop, empty ranges and argument failures preserve the unused balance.
TEST(CalendarInterval, BudgetsStopAndValidation) {
  FixedTimeZone utc(timezone::UTC);
  const TimeRange range{WallTime::Epoch(), WallTime::Epoch() + Hours(3)};
  std::vector<CalendarBoundary> actual;
  for (uint32_t budget = 0; budget <= 4; ++budget) {
    uint32_t remaining = budget;
    actual.clear();
    EXPECT_EQ(
        budget == 4 ? CalendarStatus::kOk : CalendarStatus::kLimitExceeded,
        EnumerateCalendarBoundaries(range, utc, {CalendarUnit::kHour, 1},
                                    remaining, Collect, &actual));
    EXPECT_EQ(0u, remaining);
    EXPECT_EQ(budget == 0 ? 0u : budget - 1, actual.size());
  }
  uint32_t remaining = 10;
  EXPECT_EQ(CalendarStatus::kStopped,
            EnumerateCalendarBoundaries(range, utc, {CalendarUnit::kHour, 1},
                                        remaining, Stop, nullptr));
  EXPECT_EQ(8u, remaining);
  EXPECT_EQ(CalendarStatus::kOk,
            EnumerateCalendarBoundaries(range, utc, {CalendarUnit::kHour, 1},
                                        remaining, Collect, &actual));
  EXPECT_EQ(4u, remaining);
  EXPECT_EQ(CalendarStatus::kOk,
            EnumerateCalendarBoundaries({range.end, range.end}, utc,
                                        {CalendarUnit::kHour, 1}, remaining,
                                        Collect, &actual));
  EXPECT_EQ(4u, remaining);
  EXPECT_EQ(CalendarStatus::kInvalidArgument,
            EnumerateCalendarBoundaries({range.end, range.begin}, utc,
                                        {CalendarUnit::kHour, 1}, remaining,
                                        Collect, &actual));
  EXPECT_EQ(CalendarStatus::kInvalidArgument,
            EnumerateCalendarBoundaries({WallTime::Unset(), range.end}, utc,
                                        {CalendarUnit::kHour, 1}, remaining,
                                        Collect, &actual));
  EXPECT_EQ(CalendarStatus::kInvalidArgument,
            EnumerateCalendarBoundaries(range, utc, {CalendarUnit::kHour, 1},
                                        remaining, nullptr, nullptr));
  EXPECT_EQ(CalendarStatus::kOutOfRange,
            EnumerateCalendarBoundaries({Utc(1, 1, 1) - Micros(1), range.end},
                                        utc, {CalendarUnit::kHour, 1},
                                        remaining, Collect, &actual));
  EXPECT_EQ(4u, remaining);
}

}  // namespace
}  // namespace roo_time
