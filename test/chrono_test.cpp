#include "roo_time/chrono.h"
#include "gtest/gtest.h"

static_assert(ROO_TIME_HAS_CHRONO, "Host toolchain must provide chrono");
namespace roo_time {
constexpr auto kStandard = ToChrono(Seconds(2));
static_assert(kStandard.count() == 2000000, "constexpr export");
constexpr Duration kNative = FromChrono(std::chrono::milliseconds(1500));
static_assert(kNative.inMicros() == 1500000, "constexpr import");
TEST(Chrono, UnitsAndPrecision) {
  EXPECT_EQ(1500000, ToChrono(Seconds(1) + Millis(500)).count());
  EXPECT_EQ(-1, ToChrono<std::chrono::milliseconds>(Micros(-1999)).count());
  EXPECT_DOUBLE_EQ(1.5, ToChrono<std::chrono::duration<double>>(Millis(1500)).count());
  EXPECT_EQ(-1, FromChrono(std::chrono::nanoseconds(-1999)).inMicros());
  EXPECT_EQ(1250000, FromChrono(std::chrono::duration<double>(1.25)).inMicros());
  EXPECT_EQ(1500000, FromChrono(std::chrono::duration<int, std::ratio<1, 2>>(3)).inMicros());
  SmallDuration small = Millis(250);
  EXPECT_EQ(250000, ToChrono(small).count());
  EXPECT_EQ(small, SmallDuration(FromChrono(std::chrono::milliseconds(250))));
  const Duration maximum = Micros(INT64_MAX), minimum = Micros(INT64_MIN);
  EXPECT_EQ(maximum, FromChrono(ToChrono(maximum)));
  EXPECT_EQ(minimum, FromChrono(ToChrono(minimum)));
}
}
