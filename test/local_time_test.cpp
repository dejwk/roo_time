#include <algorithm>

#include "gtest/gtest.h"
#include "roo_time/timezone.h"

namespace roo_time {
namespace {

// Creates UTC fields, also used as an offset-free representation of local
// fields.
WallTime Utc(int year, int month, int day, int hour = 0, int minute = 0) {
  return DateTime(year, month, day, hour, minute, 0, 0, timezone::UTC)
      .wallTime();
}

// Resolves the calendar fields of a timestamp without interpreting them as UTC.
LocalTimeResolution ResolveFields(WallTime fields, const TimeZone& zone) {
  const DateTime local(fields, timezone::UTC);
  return ResolveLocal(local.civilDay(), local.timeOfDay(), zone);
}

// Verifies status/field invariants and exact round trips for every returned
// instant.
void ExpectResolution(WallTime fields, const TimeZone& zone,
                      LocalTimeStatus status) {
  const DateTime local(fields, timezone::UTC);
  const LocalTimeResolution result = ResolveFields(fields, zone);
  EXPECT_EQ(status, result.status);
  if (status == LocalTimeStatus::kUnique ||
      status == LocalTimeStatus::kAmbiguous) {
    ASSERT_TRUE(result.first.isSet());
    const DateTime first = ToLocal(result.first, zone);
    EXPECT_EQ(local.civilDay(), first.civilDay());
    EXPECT_EQ(local.timeOfDay(), first.timeOfDay());
    if (status == LocalTimeStatus::kAmbiguous) {
      ASSERT_TRUE(result.second.isSet());
      EXPECT_LT(result.first, result.second);
      const DateTime second = ToLocal(result.second, zone);
      EXPECT_EQ(local.civilDay(), second.civilDay());
      EXPECT_EQ(local.timeOfDay(), second.timeOfDay());
    } else {
      EXPECT_FALSE(result.second.isSet());
    }
  } else {
    EXPECT_FALSE(result.first.isSet());
    EXPECT_FALSE(result.second.isSet());
  }
}

// Verifies exact half-open gap/overlap boundaries, including individual
// microseconds.
void CheckTransition(const TimeZone& zone, WallTime transition,
                     int before_minutes, int after_minutes) {
  const WallTime low =
      transition + Minutes(std::min(before_minutes, after_minutes));
  const WallTime high =
      transition + Minutes(std::max(before_minutes, after_minutes));
  if (before_minutes == after_minutes) {
    for (int delta : {-1, 0, 1})
      ExpectResolution(low + Micros(delta), zone, LocalTimeStatus::kUnique);
    return;
  }
  const LocalTimeStatus inside = before_minutes < after_minutes
                                     ? LocalTimeStatus::kNonexistent
                                     : LocalTimeStatus::kAmbiguous;
  ExpectResolution(low - Micros(1), zone, LocalTimeStatus::kUnique);
  ExpectResolution(low, zone, inside);
  ExpectResolution(low + (high - low) / 2, zone, inside);
  ExpectResolution(high - Micros(1), zone, inside);
  ExpectResolution(high, zone, LocalTimeStatus::kUnique);
  if (inside == LocalTimeStatus::kAmbiguous) {
    const LocalTimeResolution result =
        ResolveFields(low + Micros(123456), zone);
    EXPECT_EQ(
        low + Micros(123456) - Minutes(std::max(before_minutes, after_minutes)),
        result.first);
    EXPECT_EQ(
        low + Micros(123456) - Minutes(std::min(before_minutes, after_minutes)),
        result.second);
  }
}

// Verifies ordinary fixed offsets and local endpoints work across the full
// offset range.
TEST(LocalTime, FixedZonesAndCalendarEndpoints) {
  for (int minutes : {INT16_MIN, -330, 0, 345, INT16_MAX}) {
    const FixedTimeZone zone{UtcOffset(Minutes(minutes))};
    for (WallTime fields : {Utc(1, 1, 1), Utc(1970, 1, 1),
                            Utc(2024, 2, 29, 12, 30) + Micros(123456),
                            Utc(9999, 12, 31) + Hours(24) - Micros(1)}) {
      const LocalTimeResolution result = ResolveFields(fields, zone);
      ASSERT_EQ(LocalTimeStatus::kUnique, result.status);
      EXPECT_EQ(fields - Minutes(minutes), result.first);
      ExpectResolution(fields, zone, LocalTimeStatus::kUnique);
      const DateTime local(fields, timezone::UTC);
      EXPECT_EQ(result.first,
                zone.resolveLocal(local.civilDay(), local.timeOfDay()).first);
    }
  }
  const FixedTimeZone east{UtcOffset(Hours(1))};
  EXPECT_LT(ResolveFields(Utc(1, 1, 1), east).first, Utc(1, 1, 1));
  const FixedTimeZone west{UtcOffset(Hours(-1))};
  EXPECT_GE(ResolveFields(Utc(9999, 12, 31, 23), west).first,
            Utc(9999, 12, 31) + Hours(24));
}

// Verifies the documented autumn ambiguity and spring gap with exact UTC
// results.
TEST(LocalTime, EuropeanExamplesAndMicroseconds) {
  const RecurringTimeZone zone{UtcOffset(Hours(1)), rules::EuSummerTime()};
  const LocalTimeResolution repeated =
      ResolveLocal(CivilDay::FromYmd(2026, 10, 25),
                   TimeOfDay::FromHms(2, 30, 0, 123456), zone);
  ASSERT_EQ(LocalTimeStatus::kAmbiguous, repeated.status);
  EXPECT_EQ(Utc(2026, 10, 25, 0, 30) + Micros(123456), repeated.first);
  EXPECT_EQ(Utc(2026, 10, 25, 1, 30) + Micros(123456), repeated.second);
  ExpectResolution(Utc(2026, 3, 29, 2, 30), zone,
                   LocalTimeStatus::kNonexistent);
  ExpectResolution(Utc(2026, 7, 1, 12), zone, LocalTimeStatus::kUnique);
}

// Verifies all preset transitions, including negative and half-hour
// adjustments.
TEST(LocalTime, PresetTransitionBoundaries) {
  const RecurringTimeZone eu{UtcOffset(Hours(1)), rules::EuSummerTime()};
  CheckTransition(eu, Utc(2026, 3, 29, 1), 60, 120);
  CheckTransition(eu, Utc(2026, 10, 25, 1), 120, 60);
  const RecurringTimeZone us{UtcOffset(Hours(-5)), rules::UsDstSince2007()};
  CheckTransition(us, Utc(2026, 3, 8, 7), -300, -240);
  CheckTransition(us, Utc(2026, 11, 1, 6), -240, -300);
  const RecurringTimeZone au{UtcOffset(Hours(11)),
                             rules::AustralianEasternDst()};
  CheckTransition(au, Utc(2026, 4, 4, 16), 660, 600);
  CheckTransition(au, Utc(2026, 10, 3, 16), 600, 660);
  const RecurringTimeZone nz{UtcOffset(Hours(13)), rules::NewZealandDst()};
  CheckTransition(nz, Utc(2026, 4, 4, 14), 780, 720);
  CheckTransition(nz, Utc(2026, 9, 26, 14), 720, 780);
  const RecurringTimeZone lh{UtcOffset(Hours(11)), rules::LordHoweDst()};
  CheckTransition(lh, Utc(2026, 4, 4, 15), 660, 630);
  CheckTransition(lh, Utc(2026, 10, 3, 15, 30), 630, 660);
}

// Verifies zero adjustments produce a single candidate, even at transitions.
TEST(LocalTime, ZeroAdjustmentDoesNotDuplicate) {
  SeasonalRules rules = rules::EuSummerTime();
  rules.adjustment = timezone::UTC;
  const RecurringTimeZone zone{UtcOffset(Minutes(345)), rules};
  CheckTransition(zone, Utc(2026, 3, 29, 1), 345, 345);
  CheckTransition(zone, Utc(2026, 10, 25, 1), 345, 345);
}

// Verifies custom clock bases, end-of-day transitions, and signed adjustments.
TEST(LocalTime, CustomRulesAndEndOfDay) {
  SeasonalRules rules = {
      {AnnualDateRule::OnDay(kMarch, 1), 1440, TransitionTimeBasis::kBaseLocal},
      {AnnualDateRule::OnDay(kOctober, 1), 120,
       TransitionTimeBasis::kAdjustedLocal},
      UtcOffset(Minutes(-90))};
  const RecurringTimeZone negative{UtcOffset(Minutes(345)), rules};
  CheckTransition(negative, Utc(2026, 3, 2) - Minutes(345), 345, 255);
  CheckTransition(negative, Utc(2026, 10, 1, 2) - Minutes(255), 255, 345);
}

// Verifies transition intervals spanning local New Year use candidate UTC
// years.
TEST(LocalTime, JanuaryAndDecemberTransitions) {
  const SeasonalRules rules = {{AnnualDateRule::OnDay(kJanuary, 1), 120,
                                TransitionTimeBasis::kBaseLocal},
                               {AnnualDateRule::OnDay(kDecember, 31), 1440,
                                TransitionTimeBasis::kAdjustedLocal},
                               UtcOffset(Minutes(30))};
  const RecurringTimeZone zone{UtcOffset(Hours(2)), rules};
  CheckTransition(zone, Utc(2026, 1, 1), 120, 150);
  CheckTransition(zone, Utc(2026, 12, 31, 21, 30), 150, 120);
}

// Verifies candidates are evaluated in their UTC year even across large offset
// shifts.
TEST(LocalTime, YearCrossingsAndExtremeOffsets) {
  SeasonalRules rules = rules::EuSummerTime();
  rules.adjustment = UtcOffset(Minutes(-60));
  const RecurringTimeZone east{UtcOffset(Minutes(INT16_MAX)), rules};
  rules.adjustment = UtcOffset(Minutes(60));
  const RecurringTimeZone west{UtcOffset(Minutes(INT16_MIN)), rules};
  for (const TimeZone* zone : {static_cast<const TimeZone*>(&east),
                               static_cast<const TimeZone*>(&west)}) {
    for (WallTime instant :
         {Utc(2026, 1, 1), Utc(2026, 12, 31, 23) + Micros(1)}) {
      const DateTime local = ToLocal(instant, *zone);
      const LocalTimeResolution result =
          ResolveLocal(local.civilDay(), local.timeOfDay(), *zone);
      ASSERT_EQ(LocalTimeStatus::kUnique, result.status);
      EXPECT_EQ(instant, result.first);
      EXPECT_FALSE(result.second.isSet());
    }
  }
}

// Verifies UTC range checks precede offset queries and partial edge matches
// remain usable.
TEST(LocalTime, RecurringRangeEndpoints) {
  const RecurringTimeZone east{UtcOffset(Hours(1)), rules::EuSummerTime()};
  ExpectResolution(Utc(1, 1, 1), east, LocalTimeStatus::kOutOfRange);
  const LocalTimeResolution first = ResolveFields(Utc(1, 1, 1, 1), east);
  ASSERT_EQ(LocalTimeStatus::kUnique, first.status);
  EXPECT_EQ(Utc(1, 1, 1), first.first);
  EXPECT_FALSE(first.second.isSet());
  const RecurringTimeZone west{UtcOffset(Hours(-1)), rules::EuSummerTime()};
  const WallTime last_local = Utc(9999, 12, 31) + Hours(24) - Micros(1);
  ExpectResolution(last_local, west, LocalTimeStatus::kOutOfRange);
  const LocalTimeResolution last = ResolveFields(last_local - Hours(1), west);
  ASSERT_EQ(LocalTimeStatus::kUnique, last.status);
  EXPECT_EQ(last_local, last.first);
  EXPECT_FALSE(last.second.isSet());
}

class ForwardOnlyZone : public TimeZone {
 public:
  UtcOffset resolveOffset(WallTime) const override { return timezone::UTC; }
};

class ReverseZone : public ForwardOnlyZone {
 public:
  mutable int calls = 0;

  LocalTimeResolution resolveLocal(CivilDay date,
                                   TimeOfDay time) const override {
    ++calls;
    return FixedTimeZone(timezone::UTC).resolveLocal(date, time);
  }
};

// Verifies forward-only subclasses remain valid and opt-in overrides are
// dispatched.
TEST(LocalTime, CustomZoneCompatibilityAndDispatch) {
  const ForwardOnlyZone forward;
  const CivilDay date = CivilDay::FromYmd(1970, 1, 1);
  const TimeOfDay time = TimeOfDay::Midnight();
  const LocalTimeResolution unsupported = ResolveLocal(date, time, forward);
  EXPECT_EQ(LocalTimeStatus::kUnsupported, unsupported.status);
  EXPECT_FALSE(unsupported.first.isSet());
  EXPECT_FALSE(unsupported.second.isSet());
  ReverseZone reverse;
  const LocalTimeResolution resolved = ResolveLocal(date, time, reverse);
  EXPECT_EQ(1, reverse.calls);
  EXPECT_EQ(LocalTimeStatus::kUnique, resolved.status);
  EXPECT_EQ(WallTime::Epoch(), resolved.first);
  EXPECT_FALSE(resolved.second.isSet());
  EXPECT_EQ(LocalTimeStatus::kInvalidInput,
            ResolveLocal(CivilDay::Invalid(), time, reverse).status);
  EXPECT_EQ(1, reverse.calls);
}

// Verifies invalid fields and default results obey the status/instant
// invariants.
TEST(LocalTime, InvalidInputsAndDefaultResult) {
  const FixedTimeZone fixed{timezone::UTC};
  const RecurringTimeZone recurring{timezone::UTC, rules::EuSummerTime()};
  const ForwardOnlyZone forward;
  const TimeZone* zones[] = {&fixed, &recurring, &forward};
  for (const TimeZone* zone : zones) {
    for (CivilDay date : {CivilDay::Invalid(), CivilDay::FromYmd(2026, 1, 1)}) {
      for (TimeOfDay time : {TimeOfDay::Invalid(), TimeOfDay::Midnight()}) {
        if (date.isValid() && time.isValid()) continue;
        const LocalTimeResolution result = ResolveLocal(date, time, *zone);
        EXPECT_EQ(LocalTimeStatus::kInvalidInput, result.status);
        EXPECT_FALSE(result.first.isSet());
        EXPECT_FALSE(result.second.isSet());
        const LocalTimeResolution direct = zone->resolveLocal(date, time);
        EXPECT_EQ(LocalTimeStatus::kInvalidInput, direct.status);
        EXPECT_FALSE(direct.first.isSet());
        EXPECT_FALSE(direct.second.isSet());
      }
    }
  }
  const LocalTimeResolution result;
  EXPECT_EQ(LocalTimeStatus::kUnsupported, result.status);
  EXPECT_FALSE(result.first.isSet());
  EXPECT_FALSE(result.second.isSet());
}

// Verifies a full Gregorian cycle preserves arbitrary UTC instants under
// reverse lookup.
TEST(LocalTime, GregorianCycleRoundTrips) {
  const RecurringTimeZone north{UtcOffset(Hours(1)), rules::EuSummerTime()};
  const RecurringTimeZone south{UtcOffset(Hours(13)), rules::NewZealandDst()};
  const RecurringTimeZone half_hour{UtcOffset(Hours(11)), rules::LordHoweDst()};
  const TimeZone* zones[] = {&north, &south, &half_hour};
  for (int year = 2000; year < 2400; ++year) {
    for (int month = 1; month <= 12; ++month) {
      const int day = DaysInMonth(year, static_cast<Month>(month));
      const WallTime instant =
          Utc(year, month, day, 23, 59) + Seconds(59) + Micros(123456);
      for (const TimeZone* zone : zones) {
        const DateTime local = ToLocal(instant, *zone);
        const LocalTimeResolution result =
            ResolveLocal(local.civilDay(), local.timeOfDay(), *zone);
        ASSERT_TRUE(result.status == LocalTimeStatus::kUnique ||
                    result.status == LocalTimeStatus::kAmbiguous);
        EXPECT_TRUE(result.first == instant || result.second == instant);
        if (result.status == LocalTimeStatus::kUnique) {
          EXPECT_FALSE(result.second.isSet());
        } else {
          EXPECT_LT(result.first, result.second);
        }
      }
    }
  }
}

}  // namespace
}  // namespace roo_time
