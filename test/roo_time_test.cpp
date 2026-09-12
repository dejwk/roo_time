#include <limits>

#include "gtest/gtest.h"

#include "roo_time.h"

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
  WallTime a = WallTime(Micros(150));
  WallTime b = WallTime(Micros(27));
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
  WallTime base;
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
  DateTime d(2020, 05, 24, TimeZone(Hours(2)));
  EXPECT_EQ(2020, d.year());
  EXPECT_EQ(kMay, d.month());
  EXPECT_EQ(24, d.day());
  EXPECT_EQ(kSunday, d.dayOfWeek());
  EXPECT_EQ(145, d.dayOfYear());
  EXPECT_EQ(1590271200000000, d.wallTime().sinceEpoch().inMicros());
}

TEST(DateTime, FromDateTimeCest) {
  DateTime d(2020, 05, 25, 23, 57, 31, 1, TimeZone(Hours(2)));
  EXPECT_EQ(2020, d.year());
  EXPECT_EQ(kMay, d.month());
  EXPECT_EQ(25, d.day());
  EXPECT_EQ(kMonday, d.dayOfWeek());
  EXPECT_EQ(146, d.dayOfYear());
  EXPECT_EQ(1590443851000001, d.wallTime().sinceEpoch().inMicros());
}

TEST(DateTime, FromUnixCest) {
  DateTime d(WallTime(Micros(1590443851000001)), TimeZone(Hours(2)));
  EXPECT_EQ(2020, d.year());
  EXPECT_EQ(kMay, d.month());
  EXPECT_EQ(25, d.day());
  EXPECT_EQ(kMonday, d.dayOfWeek());
  EXPECT_EQ(146, d.dayOfYear());
  EXPECT_EQ(1590443851000001, d.wallTime().sinceEpoch().inMicros());
}

TEST(DateTime, ComparisonSemantics) {
  DateTime same_instant_different_tz(WallTime(Micros(1590443851000001)),
                                     TimeZone(Hours(2)));
  DateTime same_instant_utc(WallTime(Micros(1590443851000001)), timezone::UTC);
  EXPECT_NE(same_instant_different_tz, same_instant_utc);

  DateTime same_tz_different_instant(WallTime(Micros(1590443851000002)),
                                     TimeZone(Hours(2)));
  EXPECT_NE(same_instant_different_tz, same_tz_different_instant);
}

}  // namespace roo_time

TEST(DurationComponents, LargeValuesAndLimits) {
  using namespace roo_time;
  for (int64_t days : {0LL, 24855LL, 24856LL, 30000LL, 67108863LL}) {
    for (int sign : {-1, 1}) {
      auto duration = sign * (Hours(days * 24) + Hours(23) + Minutes(59) +
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
  for (int64_t micros : {-86400000001LL, -86400000000LL, -86399999999LL,
                         -1LL, 0LL, 1LL}) {
    for (int offset_hours : {-12, 0, 14}) {
      TimeZone tz(Hours(offset_hours));
      WallTime wall(Micros(micros));
      DateTime date(wall, tz);
      DateTime rebuilt(date.year(), date.month(), date.day(), date.hour(),
                       date.minute(), date.second(), date.micros(), tz);
      EXPECT_EQ(wall, rebuilt.wallTime());
    }
  }
  DateTime date(WallTime(Micros(-1)), timezone::UTC);
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
