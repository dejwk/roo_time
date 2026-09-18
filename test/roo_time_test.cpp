#include "roo_time.h"

#include <limits>
#include <sstream>
#include <type_traits>

#include "gtest/gtest.h"

namespace roo_time {

TEST(Duration, NarrowingConversions) {
  Duration a = Micros(12345678901);
  EXPECT_EQ(12345678, a.inMillis());
  EXPECT_EQ(12345, a.inSeconds());
  EXPECT_EQ(205, a.inMinutes());
  EXPECT_EQ(3, a.inHours());
}

TEST(Duration, ExpandingConversions) {
  Duration a = Hours(3);
  EXPECT_EQ(180, a.inMinutes());
  EXPECT_EQ(180 * 60, a.inSeconds());
  EXPECT_EQ(180 * 60 * 1000, a.inMillis());
  EXPECT_EQ(180L * 60 * 1000000, a.inMicros());
}

TEST(Duration, Arithmetics) {
  Duration a = Micros(150);
  Duration b = Micros(27);
  EXPECT_EQ(177, (a + b).inMicros());
  EXPECT_EQ(123, (a - b).inMicros());
  a += b;
  EXPECT_EQ(177, a.inMicros());
  a += b;
  EXPECT_EQ(204, a.inMicros());
  a -= b;
  EXPECT_EQ(177, a.inMicros());
}

TEST(Duration, Comparison) {
  EXPECT_EQ(Micros(150), Micros(150));
  EXPECT_FALSE(Micros(150) != Micros(150));
  EXPECT_NE(Micros(150), Micros(151));
  EXPECT_FALSE(Micros(150) == Micros(151));
  EXPECT_LE(Micros(150), Micros(150));
  EXPECT_FALSE(Micros(150) > Micros(150));
  EXPECT_GE(Micros(150), Micros(150));
  EXPECT_FALSE(Micros(150) < Micros(150));
  EXPECT_LT(Micros(139), Micros(150));
  EXPECT_FALSE(Micros(139) >= Micros(150));
  EXPECT_GT(Micros(169), Micros(150));
  EXPECT_FALSE(Micros(169) <= Micros(150));
}

TEST(Duration, RoundingSignBehavior) {
  Duration positive = Micros(1501);
  EXPECT_EQ(1, positive.inMillisRoundedDown());
  EXPECT_EQ(2, positive.inMillisRoundedUp());

  Duration negative = Micros(-1501);
  EXPECT_EQ(-1, negative.inMillisRoundedDown());
  EXPECT_EQ(-2, negative.inMillisRoundedUp());

  EXPECT_EQ(-1, Micros(-1000001).inSecondsRoundedDown());
  EXPECT_EQ(-2, Micros(-1000001).inSecondsRoundedUp());

  EXPECT_EQ(-1, Micros(-60000001).inMinutesRoundedDown());
  EXPECT_EQ(-2, Micros(-60000001).inMinutesRoundedUp());

  EXPECT_EQ(-1, Micros(-3600000001LL).inHoursRoundedDown());
  EXPECT_EQ(-2, Micros(-3600000001LL).inHoursRoundedUp());

  EXPECT_EQ(0, Micros(499).inMillisRoundedNearest());
  EXPECT_EQ(1, Micros(500).inMillisRoundedNearest());
  EXPECT_EQ(1, Micros(501).inMillisRoundedNearest());
  EXPECT_EQ(1, Micros(1499).inMillisRoundedNearest());
  EXPECT_EQ(2, Micros(1501).inMillisRoundedNearest());
  EXPECT_EQ(0, Micros(-499).inMillisRoundedNearest());
  EXPECT_EQ(-1, Micros(-500).inMillisRoundedNearest());
  EXPECT_EQ(-1, Micros(-501).inMillisRoundedNearest());
  EXPECT_EQ(-1, Micros(-1499).inMillisRoundedNearest());
  EXPECT_EQ(-2, Micros(-1501).inMillisRoundedNearest());

  EXPECT_EQ(1, Micros(500000).inSecondsRoundedNearest());
  EXPECT_EQ(-1, Micros(-500000).inSecondsRoundedNearest());

  EXPECT_EQ(1, Micros(30000000).inMinutesRoundedNearest());
  EXPECT_EQ(-1, Micros(-30000000).inMinutesRoundedNearest());

  EXPECT_EQ(1, Micros(1800000000LL).inHoursRoundedNearest());
  EXPECT_EQ(-1, Micros(-1800000000LL).inHoursRoundedNearest());
}

TEST(Uptime, NarrowingConversions) {
  Uptime a = Uptime::Start() + Micros(12345678901);
  EXPECT_EQ(12345678901, a.inMicros());
  EXPECT_EQ(12345678, a.inMillis());
  EXPECT_EQ(12345, a.inSeconds());
  EXPECT_EQ(205, a.inMinutes());
  EXPECT_EQ(3, a.inHours());
}

TEST(Uptime, ExpandingConversions) {
  Uptime a = Uptime::Start() + Hours(3);
  EXPECT_EQ(3, a.inHours());
  EXPECT_EQ(180, a.inMinutes());
  EXPECT_EQ(180 * 60, a.inSeconds());
  EXPECT_EQ(180 * 60 * 1000, a.inMillis());
  EXPECT_EQ(180L * 60 * 1000000, a.inMicros());
}

TEST(Uptime, Arithmetics) {
  Uptime a = Uptime::Start() + Micros(150);
  Uptime b = Uptime::Start() + Micros(27);
  EXPECT_EQ(123, (a - b).inMicros());
  Duration delta = Micros(13);
  EXPECT_EQ(136, (a - b + delta).inMicros());
  EXPECT_EQ(136, (a + delta - b).inMicros());

  a += delta;
  EXPECT_EQ(163, a.inMicros());
  a -= delta;
  EXPECT_EQ(150, a.inMicros());
}

TEST(Uptime, Comparison) {
  Uptime base = Uptime::Start();
  EXPECT_EQ(base + Micros(150), base + Micros(150));
  EXPECT_FALSE(base + Micros(150) != base + Micros(150));
  EXPECT_NE(base + Micros(150), base + Micros(151));
  EXPECT_FALSE(base + Micros(150) == base + Micros(151));
  EXPECT_LE(base + Micros(150), base + Micros(150));
  EXPECT_FALSE(base + Micros(150) > base + Micros(150));
  EXPECT_GE(base + Micros(150), base + Micros(150));
  EXPECT_FALSE(base + Micros(150) < base + Micros(150));
  EXPECT_LT(base + Micros(139), base + Micros(150));
  EXPECT_FALSE(base + Micros(139) >= base + Micros(150));
  EXPECT_GT(base + Micros(169), base + Micros(150));
  EXPECT_FALSE(base + Micros(169) <= base + Micros(150));
}

TEST(WallTime, Arithmetics) {
  WallTime a = WallTime::SinceEpoch(Micros(150));
  WallTime b = WallTime::SinceEpoch(Micros(27));
  EXPECT_EQ(123, (a - b).inMicros());
  Duration delta = Micros(13);
  EXPECT_EQ(136, (a - b + delta).inMicros());
  EXPECT_EQ(136, (a + delta - b).inMicros());

  a += delta;
  EXPECT_EQ(163, a.sinceEpoch().inMicros());
  a -= delta;
  EXPECT_EQ(150, a.sinceEpoch().inMicros());
}

TEST(WallTime, Comparison) {
  WallTime base = WallTime::Epoch();
  EXPECT_EQ(base + Micros(150), base + Micros(150));
  EXPECT_FALSE(base + Micros(150) != base + Micros(150));
  EXPECT_NE(base + Micros(150), base + Micros(151));
  EXPECT_FALSE(base + Micros(150) == base + Micros(151));
  EXPECT_LE(base + Micros(150), base + Micros(150));
  EXPECT_FALSE(base + Micros(150) > base + Micros(150));
  EXPECT_GE(base + Micros(150), base + Micros(150));
  EXPECT_FALSE(base + Micros(150) < base + Micros(150));
  EXPECT_LT(base + Micros(139), base + Micros(150));
  EXPECT_FALSE(base + Micros(139) >= base + Micros(150));
  EXPECT_GT(base + Micros(169), base + Micros(150));
  EXPECT_FALSE(base + Micros(169) <= base + Micros(150));
}

TEST(DateTime, FromDateUTC) {
  DateTime d(2020, 05, 24, timezone::UTC);
  EXPECT_EQ(2020, d.year());
  EXPECT_EQ(kMay, d.month());
  EXPECT_EQ(24, d.day());
  EXPECT_EQ(kSunday, d.dayOfWeek());
  EXPECT_EQ(145, d.dayOfYear());
  EXPECT_EQ(1590278400000000, d.wallTime().sinceEpoch().inMicros());
}

TEST(DateTime, FromDateCest) {
  DateTime d(2020, 05, 24, UtcOffset(Hours(2)));
  EXPECT_EQ(2020, d.year());
  EXPECT_EQ(kMay, d.month());
  EXPECT_EQ(24, d.day());
  EXPECT_EQ(kSunday, d.dayOfWeek());
  EXPECT_EQ(145, d.dayOfYear());
  EXPECT_EQ(1590271200000000, d.wallTime().sinceEpoch().inMicros());
}

TEST(DateTime, FromDateTimeCest) {
  DateTime d(2020, 05, 25, 23, 57, 31, 1, UtcOffset(Hours(2)));
  EXPECT_EQ(2020, d.year());
  EXPECT_EQ(kMay, d.month());
  EXPECT_EQ(25, d.day());
  EXPECT_EQ(kMonday, d.dayOfWeek());
  EXPECT_EQ(146, d.dayOfYear());
  EXPECT_EQ(1590443851000001, d.wallTime().sinceEpoch().inMicros());
}

TEST(DateTime, FromUnixCest) {
  DateTime d(WallTime::SinceEpoch(Micros(1590443851000001)),
             UtcOffset(Hours(2)));
  EXPECT_EQ(2020, d.year());
  EXPECT_EQ(kMay, d.month());
  EXPECT_EQ(25, d.day());
  EXPECT_EQ(kMonday, d.dayOfWeek());
  EXPECT_EQ(146, d.dayOfYear());
  EXPECT_EQ(1590443851000001, d.wallTime().sinceEpoch().inMicros());
}

TEST(DateTime, ComparisonSemantics) {
  DateTime same_instant_different_tz(
      WallTime::SinceEpoch(Micros(1590443851000001)), UtcOffset(Hours(2)));
  DateTime same_instant_utc(WallTime::SinceEpoch(Micros(1590443851000001)),
                            timezone::UTC);
  EXPECT_NE(same_instant_different_tz, same_instant_utc);

  DateTime same_tz_different_instant(
      WallTime::SinceEpoch(Micros(1590443851000002)), UtcOffset(Hours(2)));
  EXPECT_NE(same_instant_different_tz, same_tz_different_instant);
}

}  // namespace roo_time

TEST(DurationComponents, LargeValuesAndLimits) {
  using namespace roo_time;
  for (int64_t days : {0LL, 24855LL, 24856LL, 30000LL, 67108863LL}) {
    for (int sign : {-1, 1}) {
      const auto duration = sign * (Hours(days * 24) + Hours(23) + Minutes(59) +
                                    Seconds(59) + Micros(999999));
      EXPECT_EQ(duration, Duration::FromComponents(duration.toComponents()));
    }
  }
  auto minimum = Micros(std::numeric_limits<int64_t>::min());
  auto maximum = Duration::Max();
  auto min_components = minimum.toComponents();
  auto max_components = maximum.toComponents();
  EXPECT_TRUE(min_components.negative);
  EXPECT_FALSE(max_components.negative);
  EXPECT_EQ(67108863u, min_components.days);
  EXPECT_EQ(23u, min_components.hours);
  EXPECT_EQ(59u, min_components.minutes);
  EXPECT_EQ(59u, min_components.seconds);
  EXPECT_EQ(999999u, min_components.micros);
  EXPECT_EQ(Duration::FromComponents(max_components) * -1,
            Duration::FromComponents(min_components));
}

TEST(DateTime, NegativeEpochRoundTrip) {
  using namespace roo_time;
  for (int64_t micros :
       {-86400000001LL, -86400000000LL, -86399999999LL, -1LL, 0LL, 1LL}) {
    for (int offset_hours : {-12, 0, 14}) {
      UtcOffset tz(Hours(offset_hours));
      WallTime wall = WallTime::SinceEpoch(Micros(micros));
      DateTime date(wall, tz);
      DateTime rebuilt(date.year(), date.month(), date.day(), date.hour(),
                       date.minute(), date.second(), date.micros(), tz);
      EXPECT_EQ(wall, rebuilt.wallTime());
    }
  }
  DateTime date(WallTime::SinceEpoch(Micros(-1)), timezone::UTC);
  EXPECT_EQ(1969, date.year());
  EXPECT_EQ(kDecember, date.month());
  EXPECT_EQ(31, date.day());
  EXPECT_EQ(kWednesday, date.dayOfWeek());
  EXPECT_EQ(365, date.dayOfYear());
  EXPECT_EQ(23, date.hour());
  EXPECT_EQ(59, date.minute());
  EXPECT_EQ(59, date.second());
  EXPECT_EQ(999999u, date.micros());
}

#ifdef CTIME_HDR_DEFINED
TEST(DateTime, TmDayOfYear) {
  using namespace roo_time;
  EXPECT_EQ(0, DateTime(2024, 1, 1, timezone::UTC).tmStruct().tm_yday);
  EXPECT_EQ(365, DateTime(2024, 12, 31, timezone::UTC).tmStruct().tm_yday);
  EXPECT_EQ(364, DateTime(2023, 12, 31, timezone::UTC).tmStruct().tm_yday);
}
#endif

// Check source compatibility without emitting a warning from the test itself.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
static_assert(std::is_same<roo_time::TimeZone, roo_time::UtcOffset>::value,
              "The old name must remain an alias of UtcOffset");
#pragma GCC diagnostic pop

TEST(UtcOffset, ConstructionAndDateTimeIntegration) {
  using namespace roo_time;
  EXPECT_EQ(Micros(0), UtcOffset().asDuration());
  constexpr UtcOffset offset(Minutes(330));
  static_assert(offset.asDuration().inMinutes() == 330,
                "constexpr construction");
  DateTime date(2026, 9, 12, offset);
  EXPECT_EQ(Minutes(330), date.utcOffset().asDuration());
  EXPECT_EQ(date, DateTime(date.wallTime(), offset));
  EXPECT_EQ(Micros(0), timezone::UTC.asDuration());
}

// Verifies signed conversions across the complete representable minute range.
TEST(UtcOffset, UnitConversions) {
  using namespace roo_time;
  constexpr UtcOffset zero;
  static_assert(zero.inMinutes() == 0, "constexpr default construction");
  constexpr UtcOffset minimum(Minutes(INT16_MIN));
  static_assert(minimum.inMicros() == -1966080000000LL, "wide microseconds");
  static_assert(minimum.inMillis() == -1966080000L, "wide milliseconds");
  static_assert(minimum.inSeconds() == -1966080L, "wide seconds");
  for (int32_t minutes = INT16_MIN; minutes <= INT16_MAX; ++minutes) {
    const UtcOffset offset(Minutes(minutes));
    EXPECT_EQ(minutes, offset.inMinutes());
    EXPECT_EQ(minutes * 60LL, offset.inSeconds());
    EXPECT_EQ(minutes * 60000LL, offset.inMillis());
    EXPECT_EQ(minutes * 60000000LL, offset.inMicros());
    EXPECT_EQ(Minutes(minutes), offset.asDuration());
  }
}

// Verifies all comparisons use signed stored minutes, including both limits.
TEST(UtcOffset, Comparisons) {
  using namespace roo_time;
  constexpr UtcOffset negative(Minutes(-1));
  constexpr UtcOffset zero;
  static_assert(zero == timezone::UTC, "constexpr equality");
  static_assert(negative != zero, "constexpr inequality");
  static_assert(negative < zero, "constexpr less");
  static_assert(zero <= zero, "constexpr less or equal");
  static_assert(zero > negative, "constexpr greater");
  static_assert(zero >= zero, "constexpr greater or equal");
  const int32_t values[] = {INT16_MIN, -720, -1, 0, 1, 330, INT16_MAX};
  for (int32_t a : values) {
    for (int32_t b : values) {
      const UtcOffset lhs(Minutes(a));
      const UtcOffset rhs(Minutes(b));
      EXPECT_EQ(a == b, lhs == rhs);
      EXPECT_EQ(a != b, lhs != rhs);
      EXPECT_EQ(a < b, lhs < rhs);
      EXPECT_EQ(a <= b, lhs <= rhs);
      EXPECT_EQ(a > b, lhs > rhs);
      EXPECT_EQ(a >= b, lhs >= rhs);
    }
  }
}

// Verifies sub-minute input truncates toward zero before conversion/comparison.
TEST(UtcOffset, SubMinuteTruncation) {
  using namespace roo_time;
  for (int sign : {-1, 1}) {
    EXPECT_EQ(timezone::UTC, UtcOffset(Micros(sign * 59999999LL)));
    const UtcOffset offset(Micros(sign * 119999999LL));
    EXPECT_EQ(UtcOffset(Minutes(sign)), offset);
    EXPECT_EQ(Minutes(sign), offset.asDuration());
  }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// Verifies the deprecated type and accessors still agree with the new API.
TEST(UtcOffset, LegacyCompatibility) {
  using namespace roo_time;
  constexpr TimeZone legacy(Minutes(-330));
  static_assert(legacy.offset() == legacy.asDuration(), "legacy duration");
  const DateTime date(2026, 9, 12, legacy);
  EXPECT_EQ(legacy, date.timeZone());
  EXPECT_EQ(date.utcOffset(), date.timeZone());
  EXPECT_EQ(legacy.asDuration(), date.timeZone().offset());
  EXPECT_EQ(date, DateTime(date.wallTime(), date.utcOffset()));
  EXPECT_EQ(date, DateTime(2026, 9, 12, UtcOffset(Minutes(-330))));
  EXPECT_NE(date, DateTime(date.wallTime(), timezone::UTC));
}
#pragma GCC diagnostic pop

// Verifies duration endpoints and the reserved wall-time sentinel at compile
// time.
TEST(WallTime, InvalidSentinelAndLimits) {
  using namespace roo_time;
  static_assert(Duration::Min().inMicros() == INT64_MIN, "minimum duration");
  static_assert(Duration::Max().inMicros() == INT64_MAX, "maximum duration");
  constexpr WallTime invalid = WallTime::Unset();
  static_assert(!invalid.isSet(), "explicit unset is invalid");
  static_assert(!WallTime::SinceEpoch(Duration::Min()).isSet(),
                "reserved sentinel");
  static_assert(WallTime::SinceEpoch(Micros(INT64_MIN + 1)).isSet(),
                "first valid value");
  static_assert(WallTime::SinceEpoch(Duration::Max()).isSet(),
                "last valid value");
  constexpr WallTime epoch = WallTime::Epoch();
  static_assert(epoch.isSet(), "epoch remains valid");
  static_assert(epoch.sinceEpoch().inMicros() == 0, "constexpr access");
  EXPECT_EQ(invalid, WallTime::SinceEpoch(Duration::Min()));
  EXPECT_NE(invalid, epoch);
  EXPECT_EQ(WallTime::SinceEpoch(Micros(-1)), epoch - Micros(1));
  EXPECT_EQ(epoch, WallTime::SinceEpoch(Micros(-1)) + Micros(1));
  EXPECT_EQ(epoch, Micros(1) + WallTime::SinceEpoch(Micros(-1)));
  WallTime shifted = epoch;
  shifted += Seconds(1);
  shifted -= Seconds(2);
  EXPECT_EQ(Seconds(-1), shifted - epoch);
}

// Verifies the default DateTime still represents the epoch, not invalid time.
TEST(DateTime, DefaultRemainsEpoch) {
  using namespace roo_time;
  const DateTime date;
  EXPECT_TRUE(date.wallTime().isSet());
  EXPECT_EQ(WallTime::SinceEpoch(Micros(0)), date.wallTime());
  EXPECT_EQ(DateTime(1970, 1, 1, timezone::UTC), date);
  EXPECT_EQ(1970, date.year());
  EXPECT_EQ(kJanuary, date.month());
  EXPECT_EQ(1, date.day());
  EXPECT_EQ(0, date.hour());
  EXPECT_EQ(0, date.minute());
  EXPECT_EQ(0, date.second());
  EXPECT_EQ(0u, date.micros());
}

#ifndef NDEBUG
// Verifies invalid clock readings are rejected before arithmetic/conversion.
TEST(WallTimeDeathTest, RejectsInvalidArithmeticAndCalendarConversion) {
  using namespace roo_time;
  EXPECT_DEATH(
      { DateTime date(WallTime::Unset(), UtcOffset(Minutes(-1))); }, "isSet");
  EXPECT_DEATH(
      {
        WallTime value = WallTime::Unset();
        value += Seconds(1);
      },
      "isSet");
  EXPECT_DEATH(
      {
        WallTime value = WallTime::Unset();
        value -= Seconds(1);
      },
      "isSet");
  EXPECT_DEATH({ (void)(WallTime::Unset() + Seconds(1)); }, "isSet");
  EXPECT_DEATH({ (void)(Seconds(1) + WallTime::Unset()); }, "isSet");
  EXPECT_DEATH({ (void)(WallTime::Unset() - Seconds(1)); }, "isSet");
  EXPECT_DEATH(
      { (void)(WallTime::Unset() - WallTime::SinceEpoch(Micros(0))); },
      "isSet");
  EXPECT_DEATH(
      { (void)(WallTime::SinceEpoch(Micros(0)) - WallTime::Unset()); },
      "isSet");
}
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// Verifies deprecated constructors preserve the named factories' values.
TEST(WallTime, LegacyConstruction) {
  using namespace roo_time;
  constexpr WallTime default_time;
  constexpr WallTime epoch(Micros(0));
  constexpr WallTime before_epoch(Micros(-123));
  static_assert(default_time.isSet(), "legacy default is set");
  static_assert(default_time.sinceEpoch().inMicros() == 0,
                "legacy default remains the Unix epoch");
  static_assert(epoch.isSet(), "legacy epoch is set");
  static_assert(before_epoch.sinceEpoch().inMicros() == -123,
                "legacy duration construction remains constexpr");
  EXPECT_EQ(WallTime::Epoch(), default_time);
  EXPECT_EQ(WallTime::SinceEpoch(Micros(0)), default_time);
  EXPECT_EQ(Seconds(1), (default_time + Seconds(1)).sinceEpoch());
  EXPECT_EQ(DateTime(), DateTime(default_time, timezone::UTC));
  EXPECT_EQ(WallTime::Epoch(), epoch);
  EXPECT_EQ(WallTime::SinceEpoch(Micros(-123)), before_epoch);
  EXPECT_EQ(WallTime::Unset(), WallTime(Duration::Min()));
}
#pragma GCC diagnostic pop

#ifdef __linux__
// Verifies diagnostic output distinguishes unset time from the Unix epoch.
TEST(WallTime, StreamOutput) {
  using namespace roo_time;
  std::ostringstream output;
  output << WallTime::Unset() << ";" << WallTime::Epoch() << ";"
         << WallTime::SinceEpoch(Micros(-123));
  EXPECT_EQ("<unset>;0 us since Epoch;-123 us since Epoch", output.str());
}
#endif
