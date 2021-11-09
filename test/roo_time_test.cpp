#include "gtest/gtest.h"

#include "roo_time.h"

namespace roo_time {

TEST(Interval, NarrowingConversions) {
  Interval a = Micros(12345678901);
  EXPECT_EQ(12345678, a.inMillis());
  EXPECT_EQ(12345, a.inSeconds());
  EXPECT_EQ(205, a.inMinutes());
  EXPECT_EQ(3, a.inHours());
}

TEST(Interval, ExpandingConversions) {
  Interval a = Hours(3);
  EXPECT_EQ(180, a.inMinutes());
  EXPECT_EQ(180 * 60, a.inSeconds());
  EXPECT_EQ(180 * 60 * 1000, a.inMillis());
  EXPECT_EQ(180L * 60 * 1000000, a.inMicros());
}

TEST(Interval, Arithmetics) {
  Interval a = Micros(150);
  Interval b = Micros(27);
  EXPECT_EQ(177, (a + b).inMicros());
  EXPECT_EQ(123, (a - b).inMicros());
  a += b;
  EXPECT_EQ(177, a.inMicros());
  a += b;
  EXPECT_EQ(204, a.inMicros());
  a -= b;
  EXPECT_EQ(177, a.inMicros());
}

TEST(Interval, Comparison) {
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
  Interval delta = Micros(13);
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
  Interval delta = Micros(13);
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
  EXPECT_EQ(5, d.month());
  EXPECT_EQ(24, d.day());
  EXPECT_EQ(SUNDAY, d.dayOfWeek());
  EXPECT_EQ(145, d.dayOfYear());
  EXPECT_EQ(1590278400000000, d.wallTime().sinceEpoch().inMicros());
}

TEST(DateTime, FromDateCest) {
  DateTime d(2020, 05, 24, TimeZone(Hours(2)));
  EXPECT_EQ(2020, d.year());
  EXPECT_EQ(5, d.month());
  EXPECT_EQ(24, d.day());
  EXPECT_EQ(SUNDAY, d.dayOfWeek());
  EXPECT_EQ(145, d.dayOfYear());
  EXPECT_EQ(1590271200000000, d.wallTime().sinceEpoch().inMicros());
}

TEST(DateTime, FromDateTimeCest) {
  DateTime d(2020, 05, 25, 23, 57, 31, 1, TimeZone(Hours(2)));
  EXPECT_EQ(2020, d.year());
  EXPECT_EQ(5, d.month());
  EXPECT_EQ(25, d.day());
  EXPECT_EQ(MONDAY, d.dayOfWeek());
  EXPECT_EQ(146, d.dayOfYear());
  EXPECT_EQ(1590443851000001, d.wallTime().sinceEpoch().inMicros());
}

TEST(DateTime, FromUnixCest) {
  DateTime d(WallTime(Micros(1590443851000001)), TimeZone(Hours(2)));
  EXPECT_EQ(2020, d.year());
  EXPECT_EQ(5, d.month());
  EXPECT_EQ(25, d.day());
  EXPECT_EQ(MONDAY, d.dayOfWeek());
  EXPECT_EQ(146, d.dayOfYear());
  EXPECT_EQ(1590443851000001, d.wallTime().sinceEpoch().inMicros());
}

}  // namespace roo_time
