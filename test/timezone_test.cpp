#include "roo_time/timezone.h"

#include <limits>

#include "gtest/gtest.h"
#include "roo_time/format.h"

namespace roo_time {
namespace {
WallTime Utc(int y, int m, int d, int h = 0, int min = 0) {
  return DateTime(y, m, d, h, min, 0, 0, timezone::UTC).wallTime();
}

void ExpectOffset(const TimeZone& zone, WallTime t, int minutes) {
  const UtcOffset offset = zone.resolveOffset(t);
  EXPECT_EQ(minutes, offset.asDuration().inMinutes());
}

void ExpectTransition(const TimeZone& zone, WallTime t, int before, int after) {
  ExpectOffset(zone, t - Micros(1), before);
  ExpectOffset(zone, t, after);
  ExpectOffset(zone, t + Micros(1), after);
}

// Verifies northern presets switch at the exact UTC transition instants.
TEST(TimeZone, NorthernPresets) {
  RecurringTimeZone eu(UtcOffset(Hours(1)), rules::EuSummerTime());
  RecurringTimeZone us(UtcOffset(Hours(-5)), rules::UsDstSince2007());
  ExpectTransition(eu, Utc(2026, 3, 29, 1), 60, 120);
  ExpectTransition(eu, Utc(2026, 10, 25, 1), 120, 60);
  ExpectTransition(us, Utc(2026, 3, 8, 7), -300, -240);
  ExpectTransition(us, Utc(2026, 11, 1, 6), -240, -300);
}

// Verifies southern presets use a summer base and a negative winter
// adjustment, including fractional-hour changes.
TEST(TimeZone, SouthernPresetsAndNewYear) {
  RecurringTimeZone au(UtcOffset(Hours(11)), rules::AustralianEasternDst());
  RecurringTimeZone nz(UtcOffset(Hours(13)), rules::NewZealandDst());
  RecurringTimeZone lh(UtcOffset(Hours(11)), rules::LordHoweDst());
  ExpectOffset(au, Utc(2026, 1, 1), 660);
  ExpectOffset(au, Utc(2026, 7, 1), 600);
  ExpectTransition(au, Utc(2026, 4, 4, 16), 660, 600);
  ExpectTransition(au, Utc(2026, 10, 3, 16), 600, 660);
  ExpectTransition(nz, Utc(2026, 4, 4, 14), 780, 720);
  ExpectTransition(nz, Utc(2026, 9, 26, 14), 720, 780);
  ExpectTransition(lh, Utc(2026, 4, 4, 15), 660, 630);
  ExpectTransition(lh, Utc(2026, 10, 3, 15, 30), 630, 660);
}

// Verifies weekday selectors match calendar enumeration over a Gregorian cycle.
TEST(TimeZone, SelectorsOverGregorianCycle) {
  for (int year = 2000; year < 2400; ++year) {
    for (int month = 1; month <= 12; ++month) {
      for (int weekday = 0; weekday < 7; ++weekday) {
        int hits[5], count = 0;
        WallTime t = Utc(year, month, 1);
        for (int day = 1; DateTime(t, timezone::UTC).month() == month; ++day) {
          if (DateTime(t, timezone::UTC).dayOfWeek() == weekday)
            hits[count++] = day;
          t += Hours(24);
        }
        uint8_t day = 0;
        auto m = static_cast<Month>(month);
        auto w = static_cast<DayOfWeek>(weekday);
        ASSERT_TRUE(AnnualDateRule::LastWeekday(m, w).resolveDay(year, day));
        ASSERT_EQ(hits[count - 1], day);
        for (int n = 1; n <= 4; ++n) {
          ASSERT_TRUE(
              AnnualDateRule::NthWeekday(m, w, n).resolveDay(year, day));
          ASSERT_EQ(hits[n - 1], day);
        }
        ASSERT_TRUE(
            AnnualDateRule::WeekdayOnOrAfter(m, w, 15).resolveDay(year, day));
        int expected = 0;
        while (hits[expected] < 15) ++expected;
        ASSERT_EQ(hits[expected], day);
      }
    }
  }
}

// Verifies on-or-after selectors and 24:00 transitions resolve correctly.
TEST(TimeZone, OnOrAfterAndEndOfDay) {
  SeasonalRules rules = {{AnnualDateRule::WeekdayOnOrAfter(kMarch, kFriday, 23),
                          120, TransitionTimeBasis::kBaseLocal},
                         {AnnualDateRule::LastWeekday(kOctober, kThursday),
                          1440, TransitionTimeBasis::kAdjustedLocal},
                         UtcOffset(Hours(1))};
  RecurringTimeZone zone(UtcOffset(Hours(2)), rules);
  ExpectTransition(zone, Utc(2026, 3, 27), 120, 180);
  ExpectTransition(zone, Utc(2026, 10, 29, 21), 180, 120);
}

// Verifies January/December and same-month transitions need no special handling
// when they satisfy the UTC ordering and year-boundary preconditions.
TEST(TimeZone, OrderedIntervals) {
  SeasonalRules rules = {{AnnualDateRule::OnDay(kJanuary, 1), 120,
                          TransitionTimeBasis::kBaseLocal},
                         {AnnualDateRule::OnDay(kDecember, 31), 1440,
                          TransitionTimeBasis::kAdjustedLocal},
                         UtcOffset(Minutes(30))};
  RecurringTimeZone year_edges(UtcOffset(Hours(2)), rules);
  for (int year : {1, 2024, 2026, 9999}) {
    ExpectOffset(year_edges, Utc(year, 1, 1), 150);
    ExpectTransition(year_edges, Utc(year, 12, 31, 21, 30), 150, 120);
    ExpectOffset(year_edges, Utc(year, 12, 31) + Hours(24) - Micros(1), 120);
  }
  rules.first_transition = {AnnualDateRule::OnDay(kMarch, 1), 0,
                            TransitionTimeBasis::kUtc};
  rules.second_transition = {AnnualDateRule::OnDay(kMarch, 15), 0,
                             TransitionTimeBasis::kUtc};
  rules.adjustment = UtcOffset(Minutes(-30));
  RecurringTimeZone negative(UtcOffset(Minutes(345)), rules);
  ExpectTransition(negative, Utc(2026, 3, 1), 345, 315);
  ExpectTransition(negative, Utc(2026, 3, 15), 315, 345);
  rules.adjustment = UtcOffset(Minutes(0));
  RecurringTimeZone unchanged(UtcOffset(Minutes(345)), rules);
  ExpectTransition(unchanged, Utc(2026, 3, 1), 345, 345);
  ExpectTransition(unchanged, Utc(2026, 3, 15), 345, 345);
}

// Verifies invalid structural inputs preserve output values on failure.
TEST(TimeZone, InvalidRulesAndUnchangedOutputs) {
  uint8_t day = 99;
  EXPECT_FALSE(AnnualDateRule::OnDay(kFebruary, 29).resolveDay(2024, day));
  EXPECT_EQ(99, day);
  EXPECT_FALSE(AnnualDateRule::NthWeekday(kMarch, kSunday, 5).isValid());
  EXPECT_FALSE(
      AnnualDateRule::WeekdayOnOrAfter(kFebruary, kSunday, 23).isValid());
  EXPECT_FALSE(AnnualDateRule::OnDay(static_cast<Month>(0), 1).isValid());
  EXPECT_FALSE(
      AnnualDateRule::LastWeekday(kMarch, static_cast<DayOfWeek>(7)).isValid());
}

#ifndef NDEBUG
// Verifies invalid zone configurations fail immediately at construction.
TEST(TimeZoneDeathTest, InvalidConstructionAsserts) {
  SeasonalRules rules = rules::EuSummerTime();
  rules.first_transition.minutes_since_midnight = 1441;
  EXPECT_DEATH((RecurringTimeZone(timezone::UTC, rules)), "");
  rules = rules::EuSummerTime();
  rules.second_transition.date = AnnualDateRule::OnDay(kFebruary, 29);
  EXPECT_DEATH((RecurringTimeZone(timezone::UTC, rules)), "");
  rules = rules::EuSummerTime();
  rules.first_transition.basis = static_cast<TransitionTimeBasis>(99);
  EXPECT_DEATH((RecurringTimeZone(timezone::UTC, rules)), "");
  rules = rules::EuSummerTime();
  EXPECT_DEATH((RecurringTimeZone(UtcOffset(Minutes(INT16_MAX)), rules)), "");
  rules.adjustment = UtcOffset(Minutes(-1));
  EXPECT_DEATH((RecurringTimeZone(UtcOffset(Minutes(INT16_MIN)), rules)), "");
}

// Verifies debug assertions diagnose calls outside the recurring clock range.
TEST(TimeZoneDeathTest, OutOfRangeInstantAsserts) {
  RecurringTimeZone zone(timezone::UTC, rules::EuSummerTime());
  EXPECT_DEATH(zone.resolveOffset(Utc(1, 1, 1) - Micros(1)), "");
  EXPECT_DEATH(zone.resolveOffset(Utc(9999, 12, 31) + Hours(24)), "");
}
// Verifies invalid local dates assert before timestamp arithmetic can overflow.
TEST(TimeZoneDeathTest, LocalCalendarRangeAsserts) {
  FixedTimeZone fixed(UtcOffset(Hours(1)));
  EXPECT_DEATH(ToLocal(WallTime::SinceEpoch(Duration::Max()), fixed), "");
  EXPECT_DEATH(ToLocal(WallTime::SinceEpoch(Micros(INT64_MIN)), fixed), "");
  EXPECT_DEATH(ToLocal(Utc(9999, 12, 31, 23), fixed), "");
  EXPECT_DEATH(ToLocal(Utc(1, 1, 1) - Hours(1) - Micros(1), fixed), "");
}

#endif

// Verifies calendar boundaries and extreme timestamps are handled safely.
TEST(TimeZone, RangeBoundariesAndExtremeInstants) {
  RecurringTimeZone zone(timezone::UTC, rules::EuSummerTime());
  ExpectOffset(zone, Utc(1, 1, 1), 0);
  ExpectOffset(zone, Utc(9999, 12, 31, 23, 59), 0);
  RecurringTimeZone south(UtcOffset(Hours(13)), rules::NewZealandDst());
  ExpectOffset(south, Utc(1, 1, 1), 780);
  ExpectOffset(south, Utc(9999, 12, 31) + Hours(24) - Micros(1), 780);
  ExpectOffset(zone, Utc(9999, 12, 31) + Hours(24) - Micros(1), 0);
  FixedTimeZone whole_range(UtcOffset(Minutes(INT16_MIN)));
  ExpectOffset(whole_range, WallTime::SinceEpoch(Micros(INT64_MIN + 1)),
               INT16_MIN);
  ExpectOffset(whole_range, WallTime::SinceEpoch(Duration::Max()), INT16_MIN);
  FixedTimeZone fixed(UtcOffset(Hours(1)));
  const DateTime local = ToLocal(Utc(1, 1, 1) - Hours(1), fixed);
  EXPECT_EQ(1, local.year());
  EXPECT_EQ(Utc(1, 1, 1) - Hours(1), local.wallTime());
  EXPECT_EQ(Hours(1), local.utcOffset().asDuration());
  const DateTime last = ToLocal(Utc(9999, 12, 31, 23) - Micros(1), fixed);
  EXPECT_EQ(9999, last.year());
  EXPECT_EQ(23, last.hour());
  EXPECT_EQ(999999u, last.micros());
}

class CountingZone : public TimeZone {
 public:
  mutable int calls = 0;

  UtcOffset resolveOffset(WallTime) const override {
    ++calls;
    return UtcOffset(Minutes(345));
  }
};

// Verifies legacy forward-only zones preserve outputs on unsupported queries.
TEST(TimeZone, UnsupportedCalendarQueries) {
  CountingZone zone;
  TimeRange range{Utc(2000, 1, 1), Utc(2001, 1, 1)};
  OffsetSpan span{range, UtcOffset(Hours(2))};
  EXPECT_EQ(CalendarStatus::kUnsupported, zone.calendarRange(range));
  EXPECT_EQ(CalendarStatus::kUnsupported, zone.offsetSpanAt(range.begin, span));
  EXPECT_EQ(Utc(2000, 1, 1), range.begin);
  EXPECT_EQ(Utc(2001, 1, 1), range.end);
  EXPECT_EQ(range.begin, span.range.begin);
  EXPECT_EQ(range.end, span.range.end);
  EXPECT_EQ(UtcOffset(Hours(2)), span.offset);
  EXPECT_EQ(0, zone.calls);
}

// Verifies safe endpoints, maximality and preserved outputs for every offset,
// including the signed 16-bit extremes and fractional-hour configurations.
TEST(TimeZone, FixedCalendarSpans) {
  const WallTime lower = Utc(1, 1, 1);
  const WallTime upper = Utc(9999, 12, 31) + Hours(24);
  for (int minutes : {INT16_MIN, -330, 0, 345, INT16_MAX}) {
    const UtcOffset offset{Minutes(minutes)};
    FixedTimeZone zone(offset);
    TimeRange range{WallTime::Unset(), WallTime::Unset()};
    ASSERT_EQ(CalendarStatus::kOk, zone.calendarRange(range));
    EXPECT_EQ(minutes < 0 ? lower - offset.asDuration() : lower, range.begin);
    EXPECT_EQ(minutes > 0 ? upper - offset.asDuration() : upper, range.end);
    OffsetSpan span{range, timezone::UTC};
    for (WallTime instant :
         {range.begin, WallTime::Epoch(), range.end - Micros(1)}) {
      ASSERT_EQ(CalendarStatus::kOk, zone.offsetSpanAt(instant, span));
      EXPECT_EQ(range.begin, span.range.begin);
      EXPECT_EQ(range.end, span.range.end);
      EXPECT_EQ(offset, span.offset);
      const DateTime local = ToLocal(instant, zone);
      const LocalTimeResolution reverse =
          zone.resolveLocal(local.civilDay(), local.timeOfDay());
      EXPECT_EQ(LocalTimeStatus::kUnique, reverse.status);
      EXPECT_EQ(instant, reverse.first);
    }
    EXPECT_EQ(CalendarStatus::kOutOfRange,
              zone.offsetSpanAt(range.begin - Micros(1), span));
    EXPECT_EQ(CalendarStatus::kOutOfRange, zone.offsetSpanAt(range.end, span));
    EXPECT_EQ(CalendarStatus::kInvalidArgument,
              zone.offsetSpanAt(WallTime::Unset(), span));
    EXPECT_EQ(range.begin, span.range.begin);
    EXPECT_EQ(range.end, span.range.end);
    EXPECT_EQ(offset, span.offset);
  }
}

// Verifies maximal spans chain contiguously across the entire calendar range,
// including exact changes, fractional/negative adjustments and both year edges.
TEST(TimeZone, RecurringCalendarSpanChains) {
  const SeasonalRules patterns[] = {
      rules::EuSummerTime(), rules::UsDstSince2007(),
      rules::AustralianEasternDst(), rules::LordHoweDst()};
  const int bases[] = {60, -300, 660, 660};
  for (int i = 0; i < 4; ++i) {
    RecurringTimeZone zone(UtcOffset(Minutes(bases[i])), patterns[i]);
    TimeRange range{WallTime::Unset(), WallTime::Unset()};
    ASSERT_EQ(CalendarStatus::kOk, zone.calendarRange(range));
    const WallTime lower = Utc(1, 1, 1);
    const WallTime upper = Utc(9999, 12, 31) + Hours(24);
    const int adjusted = bases[i] + patterns[i].adjustment.inMinutes();
    EXPECT_EQ(bases[i] < 0 ? lower - Minutes(bases[i]) : lower, range.begin);
    const int maximum = bases[i] > adjusted ? bases[i] : adjusted;
    EXPECT_EQ(maximum > 0 ? upper - Minutes(maximum) : upper, range.end);
    WallTime cursor = range.begin;
    UtcOffset previous;
    int count = 0;
    while (cursor < range.end) {
      OffsetSpan span{range, timezone::UTC};
      ASSERT_EQ(CalendarStatus::kOk, zone.offsetSpanAt(cursor, span));
      ASSERT_EQ(cursor, span.range.begin);
      ASSERT_GT(span.range.end, cursor);
      ASSERT_LE(span.range.end, range.end);
      if (count != 0) {
        EXPECT_NE(previous, span.offset);
      }
      EXPECT_EQ(zone.resolveOffset(cursor), span.offset);
      EXPECT_EQ(zone.resolveOffset(span.range.end - Micros(1)), span.offset);
      OffsetSpan tail{range, timezone::UTC};
      ASSERT_EQ(CalendarStatus::kOk,
                zone.offsetSpanAt(span.range.end - Micros(1), tail));
      EXPECT_EQ(span.range.begin, tail.range.begin);
      EXPECT_EQ(span.range.end, tail.range.end);
      previous = span.offset;
      cursor = span.range.end;
      ASSERT_LE(++count, 20000);
    }
    EXPECT_EQ(19999, count);
    OffsetSpan saved{range, timezone::UTC};
    EXPECT_EQ(CalendarStatus::kOutOfRange,
              zone.offsetSpanAt(range.begin - Micros(1), saved));
    EXPECT_EQ(CalendarStatus::kOutOfRange, zone.offsetSpanAt(range.end, saved));
    EXPECT_EQ(CalendarStatus::kInvalidArgument,
              zone.offsetSpanAt(WallTime::Unset(), saved));
    EXPECT_EQ(range.begin, saved.range.begin);
    EXPECT_EQ(range.end, saved.range.end);
    EXPECT_EQ(timezone::UTC, saved.offset);
  }
}

// Verifies zero seasonal adjustment coalesces every rule event into one span.
TEST(TimeZone, ZeroAdjustmentCalendarSpan) {
  SeasonalRules pattern = rules::EuSummerTime();
  pattern.adjustment = timezone::UTC;
  RecurringTimeZone zone(UtcOffset(Minutes(-330)), pattern);
  TimeRange range{WallTime::Unset(), WallTime::Unset()};
  ASSERT_EQ(CalendarStatus::kOk, zone.calendarRange(range));
  for (WallTime instant :
       {range.begin, Utc(2026, 3, 29, 1), range.end - Micros(1)}) {
    OffsetSpan span{range, timezone::UTC};
    ASSERT_EQ(CalendarStatus::kOk, zone.offsetSpanAt(instant, span));
    EXPECT_EQ(range.begin, span.range.begin);
    EXPECT_EQ(range.end, span.range.end);
    EXPECT_EQ(UtcOffset(Minutes(-330)), span.offset);
  }
}

// Verifies the offset at a transition owns the new span, including when the
// first annual change is exactly the first instant of a UTC year.
TEST(TimeZone, CalendarSpanTransitionOwnership) {
  SeasonalRules pattern = {
      {AnnualDateRule::OnDay(kJanuary, 1), 0, TransitionTimeBasis::kUtc},
      {AnnualDateRule::OnDay(kDecember, 31), 0, TransitionTimeBasis::kUtc},
      UtcOffset(Minutes(30))};
  RecurringTimeZone zone(timezone::UTC, pattern);
  TimeRange range{WallTime::Unset(), WallTime::Unset()};
  ASSERT_EQ(CalendarStatus::kOk, zone.calendarRange(range));
  for (int year : {1, 2, 2026, 9999}) {
    OffsetSpan span{range, timezone::UTC};
    ASSERT_EQ(CalendarStatus::kOk, zone.offsetSpanAt(Utc(year, 1, 1), span));
    EXPECT_EQ(Utc(year, 1, 1), span.range.begin);
    EXPECT_EQ(Utc(year, 12, 31), span.range.end);
    EXPECT_EQ(UtcOffset(Minutes(30)), span.offset);
  }
}

// Verifies formatting resolves once per call.
TEST(TimeZone, FormattingResolvesOnce) {
  CountingZone zone;
  char buffer[80];
  EXPECT_EQ(TextStatus::kOk, FormatDateTime(Utc(2026, 1, 1), zone, "%F %T %:z",
                                            buffer, sizeof(buffer))
                                 .status);
  EXPECT_STREQ("2026-01-01 05:45:00 +05:45", buffer);
  EXPECT_EQ(1, zone.calls);
  EXPECT_EQ("2026-01-01T05:45:00.000000+05:45",
            FormatIsoDateTime(Utc(2026, 1, 1), zone));
  EXPECT_EQ(2, zone.calls);
  EXPECT_EQ("2026", FormatDateTime(Utc(2026, 1, 1), zone,
                                   roo::string_view("%Yignored", 2)));
  EXPECT_EQ(3, zone.calls);
  auto measured = FormatIsoDateTime(Utc(2026, 1, 1), zone, nullptr, 0);
  EXPECT_EQ(TextStatus::kBufferTooSmall, measured.status);
  EXPECT_EQ(32u, measured.size);
  EXPECT_EQ(4, zone.calls);
}

// Verifies formatting preserves offsets through gaps and repeated local times.
TEST(TimeZone, FormattingAcrossGapAndOverlap) {
  RecurringTimeZone zone(UtcOffset(Hours(1)), rules::EuSummerTime());
  EXPECT_EQ("01:59:59 +01:00",
            FormatDateTime(Utc(2026, 3, 29, 1) - Seconds(1), zone, "%T %:z"));
  EXPECT_EQ("03:00:00 +02:00",
            FormatDateTime(Utc(2026, 3, 29, 1), zone, "%T %:z"));
  EXPECT_EQ("02:30:00 +02:00",
            FormatDateTime(Utc(2026, 10, 25, 0, 30), zone, "%T %:z"));
  EXPECT_EQ("02:30:00 +01:00",
            FormatDateTime(Utc(2026, 10, 25, 1, 30), zone, "%T %:z"));
}

}  // namespace
}  // namespace roo_time

#ifndef NDEBUG
// Verifies fixed resolvers reject the newly reserved unset wall-time value.
TEST(TimeZoneDeathTest, FixedRejectsUnset) {
  roo_time::FixedTimeZone zone(roo_time::timezone::UTC);
  EXPECT_DEATH(zone.resolveOffset(roo_time::WallTime::Unset()), "isSet");
}
#endif
