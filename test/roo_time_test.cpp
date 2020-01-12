#include "gtest/gtest.h"

#include "roo_time.h"

namespace roo_time {

TEST(Interval, NarrowingConversions) {
  Interval a = Micros(12345678901);
  EXPECT_EQ(12345678, a.ToMillis());
  EXPECT_EQ(12345, a.ToSeconds());
  EXPECT_EQ(205, a.ToMinutes());
  EXPECT_EQ(3, a.ToHours());
}

TEST(Interval, ExpandingConversions) {
  Interval a = Hours(3);
  EXPECT_EQ(180, a.ToMinutes());
  EXPECT_EQ(180 * 60, a.ToSeconds());
  EXPECT_EQ(180 * 60 * 1000, a.ToMillis());
  EXPECT_EQ(180L * 60 * 1000000, a.ToMicros());
}

TEST(Interval, Arithmetics) {
  Interval a = Micros(150);
  Interval b = Micros(27);
  EXPECT_EQ(177, (a + b).ToMicros());
  EXPECT_EQ(123, (a - b).ToMicros());
  a += b;
  EXPECT_EQ(177, a.ToMicros());
  a += b;
  EXPECT_EQ(204, a.ToMicros());
  a -= b;
  EXPECT_EQ(177, a.ToMicros());
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
  EXPECT_EQ(12345678901, a.ToMicros());
  EXPECT_EQ(12345678, a.ToMillis());
  EXPECT_EQ(12345, a.ToSeconds());
  EXPECT_EQ(205, a.ToMinutes());
  EXPECT_EQ(3, a.ToHours());
}

TEST(Uptime, ExpandingConversions) {
  Uptime a = Uptime::Start() + Hours(3);
  EXPECT_EQ(3, a.ToHours());
  EXPECT_EQ(180, a.ToMinutes());
  EXPECT_EQ(180 * 60, a.ToSeconds());
  EXPECT_EQ(180 * 60 * 1000, a.ToMillis());
  EXPECT_EQ(180L * 60 * 1000000, a.ToMicros());
}

TEST(Uptime, Arithmetics) {
  Uptime a = Uptime::Start() + Micros(150);
  Uptime b = Uptime::Start() + Micros(27);
  EXPECT_EQ(123, (a - b).ToMicros());
  Interval delta = Micros(13);
  EXPECT_EQ(136, (a - b + delta).ToMicros());
  EXPECT_EQ(136, (a + delta - b).ToMicros());

  a += delta;
  EXPECT_EQ(163, a.ToMicros());
  a -= delta;
  EXPECT_EQ(150, a.ToMicros());
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

}  // namespace roo_time
