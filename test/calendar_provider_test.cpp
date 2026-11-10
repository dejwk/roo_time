#include <vector>

#include "gtest/gtest.h"
#include "roo_time/acetime.h"
#include "roo_time/calendar_interval.h"
#include "test/synthetic_calendar_zone.h"

namespace roo_time {
namespace {

WallTime Utc(int y, int m, int d, int h = 0, int minute = 0) {
  return DateTime(y, m, d, h, minute, 0, 0, timezone::UTC).wallTime();
}

bool Collect(void* context, const CalendarBoundary& value) {
  static_cast<std::vector<CalendarBoundary>*>(context)->push_back(value);
  return true;
}

bool Continue(void*, const CalendarBoundary&) { return true; }

/// Counts delegated queries to verify exact shared-budget accounting.
class CountingZone : public TimeZone {
 public:
  /// Borrows a provider for the duration of the test.
  explicit CountingZone(const TimeZone& zone) : zone_(zone) {}

  /// Delegates forward lookup.
  UtcOffset resolveOffset(WallTime instant) const override {
    return zone_.resolveOffset(instant);
  }

  /// Counts capability queries, which do not consume budget.
  CalendarStatus calendarRange(TimeRange& out) const override {
    ++ranges;
    return zone_.calendarRange(out);
  }

  /// Counts queries charged as span work.
  CalendarStatus offsetSpanAt(WallTime instant,
                              OffsetSpan& out) const override {
    ++spans;
    return zone_.offsetSpanAt(instant, out);
  }

  /// Counts queries charged as candidate work.
  LocalTimeResolution resolveLocal(CivilDay date,
                                   TimeOfDay time) const override {
    ++reverses;
    return zone_.resolveLocal(date, time);
  }

  mutable unsigned ranges = 0;
  mutable unsigned spans = 0;
  mutable unsigned reverses = 0;

 private:
  const TimeZone& zone_;
};

/// Deliberately violates one contract at a time without invoking undefined
/// behavior, so defensive traversal remains testable under sanitizers.
class BrokenZone : public TimeZone {
 public:
  enum class Fault {
    kEmptyRange,
    kUnsetRange,
    kReversedRange,
    kExtremeRange,
    kSpanUnset,
    kNoProgress,
    kSpanAfterCursor,
    kSpanOutsideRange,
    kAdjacentEqual,
    kOverlap,
    kWrongOffset,
    kUniqueUnset,
    kUniqueWrong,
    kUniqueSecondSet,
    kAmbiguousEqual,
    kAmbiguousReversed,
    kAmbiguousUnset,
    kNonexistent,
    kInvalidLocal,
    kLocalOutOfRange,
    kReverseUnsupported,
    kRangeStopped,
    kSpanStopped,
    kSpanOutOfRange,
    kSpanUnsupported
  };

  /// Selects a single invariant violation.
  explicit BrokenZone(Fault fault) : fault_(fault) {}

  /// Supplies a safe forward offset independently of malformed responses.
  UtcOffset resolveOffset(WallTime) const override { return timezone::UTC; }

  /// Returns a valid range unless testing range output or status validation.
  CalendarStatus calendarRange(TimeRange& out) const override {
    out = {WallTime::Epoch(), WallTime::Epoch() + Hours(4)};
    if (fault_ == Fault::kEmptyRange) out.end = out.begin;
    if (fault_ == Fault::kUnsetRange) out.begin = WallTime::Unset();
    if (fault_ == Fault::kReversedRange) out.end = out.begin - Micros(1);
    if (fault_ == Fault::kExtremeRange) {
      out.begin = WallTime::SinceEpoch(Micros(INT64_MIN + 1));
    }
    return fault_ == Fault::kRangeStopped ? CalendarStatus::kStopped
                                          : CalendarStatus::kOk;
  }

  /// Returns malformed spans or unexpected query statuses.
  CalendarStatus offsetSpanAt(WallTime instant,
                              OffsetSpan& out) const override {
    ++spans;
    out = {{WallTime::Epoch(), WallTime::Epoch() + Hours(4)}, timezone::UTC};
    if (fault_ == Fault::kSpanUnset) out.range.end = WallTime::Unset();
    if (fault_ == Fault::kNoProgress) out.range.end = instant;
    if (fault_ == Fault::kSpanAfterCursor) {
      out.range.begin = instant + Micros(1);
    }
    if (fault_ == Fault::kSpanOutsideRange) out.range.begin -= Micros(1);
    if (fault_ == Fault::kWrongOffset) out.offset = UtcOffset(Hours(1));
    if (fault_ == Fault::kAdjacentEqual || fault_ == Fault::kOverlap) {
      if (spans == 1) {
        out.range.end = WallTime::Epoch() + Hours(1);
      } else {
        if (fault_ == Fault::kAdjacentEqual) {
          out.range.begin = instant;
        } else {
          out.offset = UtcOffset(Hours(1));
        }
      }
    }
    if (fault_ == Fault::kSpanStopped) return CalendarStatus::kStopped;
    if (fault_ == Fault::kSpanOutOfRange) return CalendarStatus::kOutOfRange;
    if (fault_ == Fault::kSpanUnsupported) return CalendarStatus::kUnsupported;
    return CalendarStatus::kOk;
  }

  /// Returns one malformed reverse result for retained calendar candidates.
  LocalTimeResolution resolveLocal(CivilDay date,
                                   TimeOfDay time) const override {
    const FixedTimeZone utc(timezone::UTC);
    LocalTimeResolution result = utc.resolveLocal(date, time);
    switch (fault_) {
      case Fault::kUniqueUnset:
        result.first = WallTime::Unset();
        break;
      case Fault::kUniqueWrong:
        result.first += Seconds(1);
        break;
      case Fault::kUniqueSecondSet:
        result.second = result.first + Seconds(1);
        break;
      case Fault::kAmbiguousEqual:
        result.status = LocalTimeStatus::kAmbiguous;
        result.second = result.first;
        break;
      case Fault::kAmbiguousReversed:
        result.status = LocalTimeStatus::kAmbiguous;
        result.second = result.first - Seconds(1);
        break;
      case Fault::kAmbiguousUnset:
        result.status = LocalTimeStatus::kAmbiguous;
        result.second = WallTime::Unset();
        break;
      case Fault::kNonexistent:
        return {LocalTimeStatus::kNonexistent};
      case Fault::kInvalidLocal:
        return {LocalTimeStatus::kInvalidInput};
      case Fault::kLocalOutOfRange:
        return {LocalTimeStatus::kOutOfRange};
      case Fault::kReverseUnsupported:
        return {LocalTimeStatus::kUnsupported};
      default:
        break;
    }
    return result;
  }

  mutable unsigned spans = 0;

 private:
  Fault fault_;
};

// Verifies multiple irregular changes between equal endpoint offsets cannot be
// missed, and reverse classification includes matches outside the viewport.
TEST(CalendarProvider, MultipleIrregularTransitions) {
  using testing::SyntheticCalendarZone;
  const SyntheticCalendarZone::Change changes[] = {
      {Utc(2026, 1, 1, 0, 7), UtcOffset(Minutes(30))},
      {Utc(2026, 1, 1, 1, 13), timezone::UTC},
      {Utc(2026, 1, 1, 3, 5), UtcOffset(Minutes(-30))},
      {Utc(2026, 1, 1, 4, 47), timezone::UTC}};
  const TimeRange range{Utc(2026, 1, 1), Utc(2026, 1, 1, 6)};
  SyntheticCalendarZone table(range, timezone::UTC, changes, 4);
  CountingZone zone(table);
  std::vector<CalendarBoundary> actual;
  uint32_t remaining = 100;
  ASSERT_EQ(CalendarStatus::kOk, EnumerateCalendarBoundaries(
                                     range, zone, {CalendarUnit::kMinute, 20},
                                     remaining, Collect, &actual));
  const int expected_minutes[] = {0,   10,  30,  50,  70,  80,  100,
                                  120, 140, 160, 180, 190, 210, 230,
                                  250, 270, 300, 320, 340};
  ASSERT_EQ(19u, actual.size());
  for (size_t i = 0; i < actual.size(); ++i) {
    EXPECT_EQ(range.begin + Minutes(expected_minutes[i]), actual[i].instant);
    const DateTime local(actual[i].instant, actual[i].offset);
    EXPECT_EQ(0, local.minute() % 20);
    EXPECT_EQ(zone.resolveOffset(actual[i].instant), actual[i].offset);
    if (i != 0) {
      EXPECT_GT(actual[i].instant, actual[i - 1].instant);
    }
  }
  EXPECT_EQ(LocalOccurrence::kEarlier, actual[3].occurrence);  // Local 01:20.
  EXPECT_EQ(LocalOccurrence::kLater, actual[5].occurrence);
  EXPECT_EQ(LocalOccurrence::kEarlier, actual[9].occurrence);  // Local 02:40.
  EXPECT_EQ(LocalOccurrence::kLater, actual[11].occurrence);
  EXPECT_EQ(5u, zone.spans);
  EXPECT_EQ(19u, zone.reverses);
  EXPECT_EQ(76u, remaining);
  actual.clear();
  ASSERT_EQ(CalendarStatus::kOk,
            EnumerateCalendarBoundaries(
                {Utc(2026, 1, 1, 1, 15), Utc(2026, 1, 1, 1, 21)}, zone,
                {CalendarUnit::kMinute, 20}, remaining, Collect, &actual));
  ASSERT_EQ(1u, actual.size());
  EXPECT_EQ(LocalOccurrence::kLater, actual[0].occurrence);
}

// Verifies every unit of work is charged before its provider call, including
// spans containing no candidates; capability queries consume no work.
TEST(CalendarProvider, ExactBudgetAccounting) {
  using testing::SyntheticCalendarZone;
  const TimeRange range{Utc(2026, 6, 1), Utc(2026, 6, 1, 6)};
  const SyntheticCalendarZone::Change changes[] = {
      {range.begin + Minutes(7), UtcOffset(Minutes(30))},
      {range.begin + Minutes(73), timezone::UTC},
      {range.begin + Minutes(185), UtcOffset(Minutes(-30))},
      {range.begin + Minutes(287), timezone::UTC}};
  SyntheticCalendarZone table(range, timezone::UTC, changes, 4);
  for (CalendarUnit unit : {CalendarUnit::kMinute, CalendarUnit::kYear}) {
    for (uint32_t budget = 0; budget <= 50; ++budget) {
      CountingZone zone(table);
      uint32_t remaining = budget;
      const CalendarStatus status = EnumerateCalendarBoundaries(
          range, zone, {unit, 1}, remaining, Continue, nullptr);
      EXPECT_EQ(1u, zone.ranges);
      EXPECT_EQ(budget - remaining, zone.spans + zone.reverses);
      EXPECT_LE(zone.spans + zone.reverses, budget);
      if (unit == CalendarUnit::kYear) {
        EXPECT_EQ(
            budget >= 5 ? CalendarStatus::kOk : CalendarStatus::kLimitExceeded,
            status);
        EXPECT_EQ(0u, zone.reverses);
      } else {
        EXPECT_EQ(CalendarStatus::kLimitExceeded, status);
      }
    }
  }
  RecurringTimeZone recurring(UtcOffset(Hours(1)), rules::EuSummerTime());
  CountingZone zone(recurring);
  TimeRange centuries{WallTime::Unset(), WallTime::Unset()};
  ASSERT_EQ(CalendarStatus::kOk, recurring.calendarRange(centuries));
  uint32_t remaining = 100;
  EXPECT_EQ(
      CalendarStatus::kLimitExceeded,
      EnumerateCalendarBoundaries(centuries, zone, {CalendarUnit::kYear, 9999},
                                  remaining, Continue, nullptr));
  EXPECT_EQ(100u, zone.spans);
  EXPECT_EQ(0u, zone.reverses);
}

// Verifies malformed ranges, spans, status codes and reverse results fail
// explicitly, preserving the balance left after the work already performed.
TEST(CalendarProvider, RejectsBrokenContracts) {
  using Fault = BrokenZone::Fault;
  for (Fault fault : {Fault::kEmptyRange,      Fault::kUnsetRange,
                      Fault::kReversedRange,   Fault::kExtremeRange,
                      Fault::kSpanUnset,       Fault::kNoProgress,
                      Fault::kSpanAfterCursor, Fault::kSpanOutsideRange,
                      Fault::kAdjacentEqual,   Fault::kOverlap,
                      Fault::kWrongOffset,     Fault::kUniqueUnset,
                      Fault::kUniqueWrong,     Fault::kUniqueSecondSet,
                      Fault::kAmbiguousEqual,  Fault::kAmbiguousReversed,
                      Fault::kAmbiguousUnset,  Fault::kNonexistent,
                      Fault::kInvalidLocal,    Fault::kLocalOutOfRange,
                      Fault::kRangeStopped,    Fault::kSpanStopped,
                      Fault::kSpanOutOfRange}) {
    BrokenZone zone(fault);
    uint32_t remaining = 20;
    EXPECT_EQ(CalendarStatus::kProviderError,
              EnumerateCalendarBoundaries(
                  {WallTime::Epoch(), WallTime::Epoch() + Hours(2)}, zone,
                  {CalendarUnit::kHour, 1}, remaining, Continue, nullptr))
        << static_cast<int>(fault);
    EXPECT_GE(remaining, 17u);
  }
}

// Verifies unsupported capability, span and reverse lookup never become UTC
// output; the optional AceTime adapter retains its unsupported contract.
TEST(CalendarProvider, UnsupportedCapabilities) {
  AceTimeZone ace(ace_time::TimeZone(3600));
  uint32_t remaining = 20;
  EXPECT_EQ(CalendarStatus::kUnsupported,
            EnumerateCalendarBoundaries(
                {WallTime::Epoch(), WallTime::Epoch() + Hours(2)}, ace,
                {CalendarUnit::kHour, 1}, remaining, Continue, nullptr));
  EXPECT_EQ(20u, remaining);
  for (BrokenZone::Fault fault : {BrokenZone::Fault::kReverseUnsupported,
                                  BrokenZone::Fault::kSpanUnsupported}) {
    BrokenZone zone(fault);
    EXPECT_EQ(CalendarStatus::kUnsupported,
              EnumerateCalendarBoundaries(
                  {WallTime::Epoch(), WallTime::Epoch() + Hours(2)}, zone,
                  {CalendarUnit::kHour, 1}, remaining, Continue, nullptr));
  }
  EXPECT_EQ(17u, remaining);
}

// Verifies positive half-hour changes with a fractional-hour base align to
// local hours rather than polling on whole UTC hours.
TEST(CalendarProvider, PositiveHalfHourWithFractionalBase) {
  SeasonalRules pattern = rules::EuSummerTime();
  pattern.adjustment = UtcOffset(Minutes(30));
  RecurringTimeZone zone(UtcOffset(Minutes(345)), pattern);
  std::vector<CalendarBoundary> actual;
  uint32_t remaining = 100;
  ASSERT_EQ(CalendarStatus::kOk,
            EnumerateCalendarBoundaries({Utc(2026, 3, 29), Utc(2026, 3, 29, 3)},
                                        zone, {CalendarUnit::kHour, 1},
                                        remaining, Collect, &actual));
  ASSERT_EQ(3u, actual.size());
  EXPECT_EQ(Utc(2026, 3, 29, 0, 15), actual[0].instant);
  EXPECT_EQ(Utc(2026, 3, 29, 1, 45), actual[1].instant);
  EXPECT_EQ(Utc(2026, 3, 29, 2, 45), actual[2].instant);
}

// Verifies invalid input is rejected before querying a provider and that even
// extreme WallTime endpoints cannot overflow the range preflight arithmetic.
TEST(CalendarProvider, PreflightPreservesBudget) {
  FixedTimeZone utc(timezone::UTC);
  CountingZone zone(utc);
  const TimeRange range{WallTime::Epoch(), WallTime::Epoch() + Hours(2)};
  uint32_t remaining = 7;
  for (CalendarInterval interval :
       {CalendarInterval{CalendarUnit::kHour, 24},
        CalendarInterval{CalendarUnit::kMinute, 7},
        CalendarInterval{CalendarUnit::kYear, 0},
        CalendarInterval::Days(1, CivilDay()),
        CalendarInterval::Weeks(static_cast<DayOfWeek>(7))}) {
    EXPECT_EQ(CalendarStatus::kInvalidArgument,
              EnumerateCalendarBoundaries(range, zone, interval, remaining,
                                          Continue, nullptr));
  }
  EXPECT_EQ(CalendarStatus::kInvalidArgument,
            EnumerateCalendarBoundaries({range.begin, WallTime::Unset()}, zone,
                                        {CalendarUnit::kHour, 1}, remaining,
                                        Continue, nullptr));
  EXPECT_EQ(0u, zone.ranges);
  const WallTime low = WallTime::SinceEpoch(Micros(INT64_MIN + 1));
  const WallTime high = WallTime::SinceEpoch(Micros(INT64_MAX));
  EXPECT_EQ(
      CalendarStatus::kOutOfRange,
      EnumerateCalendarBoundaries({low, high}, zone, {CalendarUnit::kHour, 1},
                                  remaining, Continue, nullptr));
  EXPECT_EQ(7u, remaining);
  EXPECT_EQ(0u, zone.spans);
  remaining = 0;
  EXPECT_EQ(
      CalendarStatus::kOk,
      EnumerateCalendarBoundaries({high, high}, zone, {CalendarUnit::kHour, 1},
                                  remaining, Continue, nullptr));
  EXPECT_EQ(0u, zone.spans);
}

// Verifies offset extremes, very long steps and each interval unit stay safe
// at both calendar endpoints, including an unconvertible exclusive sentinel.
TEST(CalendarProvider, ExtremeOffsetsAndCalendarEdges) {
  const CalendarInterval intervals[] = {{CalendarUnit::kMillisecond, 500},
                                        {CalendarUnit::kSecond, 30},
                                        {CalendarUnit::kMinute, 30},
                                        {CalendarUnit::kHour, 12},
                                        CalendarInterval::Days(65535),
                                        CalendarInterval::Weeks(kSunday, 65535),
                                        {CalendarUnit::kMonth, 6},
                                        {CalendarUnit::kYear, 9999}};
  for (int minutes : {INT16_MIN, -345, 0, 345, INT16_MAX - 30}) {
    FixedTimeZone fixed{UtcOffset(Minutes(minutes))};
    SeasonalRules pattern = rules::EuSummerTime();
    pattern.adjustment = UtcOffset(Minutes(30));
    RecurringTimeZone recurring(UtcOffset(Minutes(minutes)), pattern);
    for (const TimeZone* zone : {static_cast<const TimeZone*>(&fixed),
                                 static_cast<const TimeZone*>(&recurring)}) {
      TimeRange supported{WallTime::Unset(), WallTime::Unset()};
      ASSERT_EQ(CalendarStatus::kOk, zone->calendarRange(supported));
      const TimeRange requests[] = {
          {supported.begin, supported.begin + Micros(2)},
          {supported.end - Micros(2), supported.end}};
      for (TimeRange request : requests) {
        for (CalendarInterval interval : intervals) {
          uint32_t remaining = 10;
          std::vector<CalendarBoundary> actual;
          EXPECT_EQ(CalendarStatus::kOk,
                    EnumerateCalendarBoundaries(request, *zone, interval,
                                                remaining, Collect, &actual));
          for (const CalendarBoundary& boundary : actual) {
            const DateTime local(boundary.instant, boundary.offset);
            EXPECT_GE(local.year(), 1);
            EXPECT_LE(local.year(), 9999);
            EXPECT_GE(boundary.instant, request.begin);
            EXPECT_LT(boundary.instant, request.end);
          }
        }
      }
    }
  }
}

}  // namespace
}  // namespace roo_time
