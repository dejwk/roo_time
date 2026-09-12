#include <tuple>
#include <type_traits>

#include "roo_time.h"
#include "gtest/gtest.h"

namespace roo_time {
namespace {
static_assert(sizeof(SmallDuration) == sizeof(int32_t), "Compact duration");
static_assert(sizeof(SmallTimestamp) == sizeof(uint32_t), "Compact timestamp");
static_assert(sizeof(Duration) == sizeof(int64_t),
              "Shared accessors add no storage");
static_assert(std::is_trivially_copyable<SmallDuration>::value, "Value type");
static_assert(std::is_trivially_copyable<SmallTimestamp>::value, "Value type");
static_assert(std::is_convertible<Uptime, SmallTimestamp>::value,
              "Narrow timestamp");
static_assert(!std::is_convertible<SmallTimestamp, Uptime>::value,
              "No epoch recovery");
static_assert(std::is_convertible<SmallDuration, Duration>::value,
              "Lossless widening");
static_assert(!std::is_convertible<Duration, SmallDuration>::value,
              "Explicit narrowing");
static_assert(!std::is_convertible<int, SmallDuration>::value,
              "Units required");
static_assert(!std::is_convertible<uint32_t, SmallTimestamp>::value,
              "Opaque ticks");
static_assert(std::is_same<decltype(Seconds(0.5)), Duration>::value,
              "Floating compatibility");
constexpr Duration kLargeSum = Millis(2000000000) + Millis(2000000000);
static_assert(kLargeSum.inMillis() == 4000000000LL, "Widen before arithmetic");
constexpr Duration kLargeSeconds = Seconds(3000000);
static_assert(kLargeSeconds.inMicros() == 3000000000000LL,
              "Widen before scaling");
constexpr SmallDuration kFiveSeconds = SmallSeconds(5);
static_assert(kFiveSeconds.inMillis() == 5000,
              "Destination selects compact storage");
static_assert(
    std::is_same<decltype(kFiveSeconds + kFiveSeconds), SmallDuration>::value,
    "Compact arithmetic preserves its range");
static_assert(std::is_same<decltype(kFiveSeconds + Millis(5)), Duration>::value,
              "Helper arithmetic widens");


template <typename A, typename B, typename = void>
struct CanMultiply : std::false_type {};
template <typename A, typename B>
struct CanMultiply<A, B, std::void_t<decltype(std::declval<A>() * std::declval<B>())>>
    : std::true_type {};
template <typename T, typename = void>
struct CanMakeSmall : std::false_type {};
template <typename T>
struct CanMakeSmall<T, std::void_t<decltype(SmallMillis(std::declval<T>())),
    decltype(SmallSeconds(std::declval<T>())),
    decltype(SmallMinutes(std::declval<T>())),
    decltype(SmallHours(std::declval<T>()))>> : std::true_type {};
static_assert(!CanMakeSmall<double>::value && !CanMakeSmall<float>::value,
              "Compact factories reject floating counts");
static_assert(CanMakeSmall<int64_t>::value && CanMakeSmall<uint64_t>::value,
              "Compact factories accept wide integers");
static_assert(CanMultiply<Duration, double>::value &&
              CanMultiply<double, Duration>::value &&
              CanMultiply<SmallDuration, float>::value &&
              CanMultiply<float, SmallDuration>::value,
              "Floating factors are supported in either order");
static_assert(std::is_same<decltype(Seconds(2)), Duration>::value &&
              std::is_same<decltype(SmallSeconds(2)), SmallDuration>::value,
              "Factories have predictable return types");
static_assert((Micros(-1) * uint64_t{4294967296}).inMicros() == -4294967296LL,
              "Unsigned factors preserve width and duration sign");
static_assert((int64_t{4294967296} * Micros(1)).inMicros() == 4294967296LL,
              "Wide factors work in either order");
static_assert((SmallMillis(1) * int64_t{65536}).inMillis() == 65536 &&
              (uint64_t{65536} * SmallMillis(-1)).inMillis() == -65536,
              "Compact multiplication does not narrow through int");
static_assert(std::is_same<decltype(SmallSeconds(1) * int64_t{2}),
                           SmallDuration>::value,
              "Compact multiplication stays compact");
static_assert(std::is_same<decltype(SmallSeconds(1) + Seconds(1)), Duration>::value,
              "Mixed addition widens");

template <typename Rep> void CheckFloatingMultiplication() {
  constexpr Rep half = Rep(0.5);
  static_assert((Seconds(2) * half).inMicros() == 1000000,
                "Multiply before truncating");
  static_assert((half * SmallMillis(3)).inMillis() == 1,
                "Compact result truncates to milliseconds");
  static_assert(std::is_same<decltype(half * Seconds(2)), Duration>::value,
                "Full multiplication stays full");
  static_assert(std::is_same<decltype(SmallMillis(3) * half), SmallDuration>::value,
                "Floating compact multiplication stays compact");
  for (int sign : {-1, 1}) {
    EXPECT_EQ(sign, (Micros(sign * 3) * half).inMicros());
    EXPECT_EQ(sign, (half * Micros(sign * 3)).inMicros());
    EXPECT_EQ(-sign, (Micros(sign * 3) * -half).inMicros());
    EXPECT_EQ(-sign, (-half * Micros(sign * 3)).inMicros());
    EXPECT_EQ(sign, (SmallMillis(sign * 3) * half).inMillis());
    EXPECT_EQ(sign, (half * SmallMillis(sign * 3)).inMillis());
    EXPECT_EQ(-sign, (SmallMillis(sign * 3) * -half).inMillis());
    EXPECT_EQ(-sign, (-half * SmallMillis(sign * 3)).inMillis());
  }
  EXPECT_EQ(0, (Micros(1) * half).inMicros());
  EXPECT_EQ(0, (SmallMillis(-1) * half).inMillis());
  EXPECT_EQ(0, (Seconds(2) * Rep(0)).inMicros());
  EXPECT_EQ(0, (Rep(0) * SmallSeconds(2)).inMillis());
  EXPECT_EQ(5000000, (Seconds(2) * Rep(2.5)).inMicros());
  EXPECT_EQ(5000, (Rep(2.5) * SmallSeconds(2)).inMillis());
}

TEST(DurationMultiplication, FloatingFactors) {
  CheckFloatingMultiplication<float>();
  CheckFloatingMultiplication<double>();
  CheckFloatingMultiplication<long double>();
  // Integer factors must retain the exact integer path above double precision.
  EXPECT_EQ(9007199254740993LL, (Micros(9007199254740993LL) * 1).inMicros());
#ifndef NDEBUG
  EXPECT_DEATH({ (void)(SmallMillis(INT32_MAX) * 2.0); }, "");
#endif
}

TEST(DurationFactories, CompactUnitsAndBoundaries) {
  EXPECT_EQ(250, SmallMillis(250).inMillis());
  EXPECT_EQ(-2000, SmallSeconds(-2).inMillis());
  EXPECT_EQ(120000, SmallMinutes(2).inMillis());
  EXPECT_EQ(7200000, SmallHours(2).inMillis());
  EXPECT_EQ(INT32_MIN, SmallMillis(INT32_MIN).inMillis());
  EXPECT_EQ(INT32_MAX, SmallMillis(uint64_t{INT32_MAX}).inMillis());
  EXPECT_EQ(-2147483000, SmallSeconds(-2147483).inMillis());
  EXPECT_EQ(2145600000, SmallHours(596).inMillis());
  auto mutable_duration = Seconds(2);
  mutable_duration += Millis(500);
  EXPECT_EQ(2500, mutable_duration.inMillis());
  EXPECT_EQ(Seconds(3), SmallSeconds(1) + Seconds(2));
  EXPECT_EQ(Seconds(1), Seconds(2) - SmallSeconds(1));
  EXPECT_LT(SmallSeconds(1), Seconds(2));
#ifndef NDEBUG
  EXPECT_DEATH({ (void)SmallMillis(uint64_t{UINT32_MAX}); }, "");
  EXPECT_DEATH({ (void)SmallSeconds(2147484); }, "");
  EXPECT_DEATH({ (void)SmallMinutes(-35792); }, "");
  EXPECT_DEATH({ (void)SmallHours(597); }, "");
  EXPECT_DEATH({ (void)SmallHours(UINT64_MAX); }, "");
#endif
}

SmallTimestamp At(uint32_t millis) { return Uptime::Start() + Millis(millis); }

template <typename A, typename B> void CheckPair(A a, B b) {
  const Duration da = a, db = b;
  static_assert(std::is_same<decltype(a + b), Duration>::value, "Wide result");
  EXPECT_EQ((a + b).inMicros(), da.inMicros() + db.inMicros());
  EXPECT_EQ((a - b).inMicros(), da.inMicros() - db.inMicros());
  EXPECT_EQ(a == b, da.inMicros() == db.inMicros());
  EXPECT_EQ(a != b, da.inMicros() != db.inMicros());
  EXPECT_EQ(a < b, da.inMicros() < db.inMicros());
  EXPECT_EQ(a <= b, da.inMicros() <= db.inMicros());
  EXPECT_EQ(a > b, da.inMicros() > db.inMicros());
  EXPECT_EQ(a >= b, da.inMicros() >= db.inMicros());
  EXPECT_EQ(a + db, da + b);
  EXPECT_EQ(db + a, b + da);
  EXPECT_EQ(a * 3, da * 3);
  EXPECT_EQ(3 * a, 3 * da);
}

template <typename A, typename Tuple> void CheckRow(A a, const Tuple &values) {
  std::apply([&](auto... b) { (CheckPair(a, b), ...); }, values);
}

TEST(DurationFactories, AllUnitPairsAndMixedDurations) {
  auto values =
      std::make_tuple(Micros(7), Millis(11), Seconds(3), Minutes(2), Hours(1));
  std::apply([&](auto... a) { (CheckRow(a, values), ...); }, values);
  CheckPair(SmallDuration::Millis(500), Seconds(2));
  CheckPair(Seconds(2), SmallDuration::Millis(500));
  struct ConvertibleDuration {
    operator Duration() const { return Millis(4); }
  };
  EXPECT_EQ(Millis(7), ConvertibleDuration() + Millis(3));
  Duration accumulated = Seconds(1);
  accumulated += Millis(500);
  accumulated -= Micros(1);
  EXPECT_EQ(1499999, accumulated.inMicros());
}

TEST(DurationFactories, SharedAccessorsAndConversions) {
  const auto negative = Micros(-1501);
  EXPECT_EQ(-1501, negative.inMicros());
  EXPECT_EQ(-1, Micros(-1.5).inMicros());
  EXPECT_EQ(-1, negative.inMillis());
  EXPECT_EQ(-1, negative.inMillisRoundedDown());
  EXPECT_EQ(-2, negative.inMillisRoundedUp());
  EXPECT_EQ(-2, negative.inMillisRoundedNearest());
  EXPECT_EQ(0, negative.inSeconds());
  EXPECT_EQ(-1, negative.inSecondsRoundedUp());
  EXPECT_FLOAT_EQ(-1.501f, negative.inMillisFloat());
  EXPECT_EQ(60, Hours(1).inMinutes());
  EXPECT_FLOAT_EQ(0.5f, Minutes(30).inHoursFloat());
  EXPECT_EQ(Minutes(3), Duration::FromComponents(Minutes(3).toComponents()));
  const SmallDuration truncated(Micros(-1999));
  EXPECT_EQ(-1, truncated.inMillis());
  EXPECT_EQ(-1000, truncated.inMicros());
  EXPECT_EQ(-1, truncated.inSecondsRoundedUp());
  EXPECT_EQ(Millis(INT32_MIN), Duration(SmallDuration::Millis(INT32_MIN)));
  EXPECT_EQ(Millis(INT32_MAX), Duration(SmallDuration::Millis(INT32_MAX)));
  EXPECT_EQ(INT32_MAX, SmallDuration(Millis(INT32_MAX)).inMillis());
  EXPECT_EQ(INT64_MAX, Micros(INT64_MAX).inMicros());
  EXPECT_EQ(INT64_MIN, Micros(INT64_MIN).inMicros());
  EXPECT_EQ(Seconds(uint32_t{3000000}), kLargeSeconds);
}

TEST(SmallDuration, ArithmeticAndExplicitNarrowing) {
  SmallDuration a = SmallSeconds(2), b = SmallMillis(500);
  EXPECT_EQ(2500, (a + b).inMillis());
  EXPECT_EQ(1500, (a - b).inMillis());
  EXPECT_EQ(4000, (a * 2).inMillis());
  EXPECT_EQ(4000, (2 * a).inMillis());
  a += b;
  a -= SmallMillis(1);
  EXPECT_EQ(2499, a.inMillis());
  const Duration full = Micros(2500999);
  EXPECT_EQ(2500, SmallDuration(full).inMillis());
  EXPECT_EQ(Millis(2499), Duration::FromComponents(a.toComponents()));
}

TEST(SmallTimestamp, WraparoundOrderingAndArithmetic) {
  const auto zero = At(0), last = At(UINT32_MAX);
  EXPECT_GT(zero, last);
  EXPECT_LT(last, zero);
  EXPECT_EQ(1, (zero - last).inMillis());
  EXPECT_EQ(-1, (last - zero).inMillis());
  EXPECT_EQ(zero, last + Millis(1));
  EXPECT_EQ(zero, Millis(1) + last);
  EXPECT_EQ(last, zero - Millis(1));
  auto t = last;
  t += Seconds(1);
  EXPECT_EQ(At(999), t);
  t -= Millis(1000);
  EXPECT_EQ(last, t);
  EXPECT_EQ(0, (t - t).inMillis());
  EXPECT_LE(t, t);
  EXPECT_GE(t, t);
  EXPECT_FALSE(t != t);
  EXPECT_NE(t, zero);
  EXPECT_EQ(INT32_MAX, (At(INT32_MAX) - zero).inMillis());
  EXPECT_EQ(-INT32_MAX, (zero - At(INT32_MAX)).inMillis());
  const SmallTimestamp repeated = Uptime::Start() + Millis(0x100000000LL);
  EXPECT_EQ(zero, repeated);
  EXPECT_EQ(At(1), SmallTimestamp(Uptime::Start() + Micros(1999)));
  const auto before = SmallTimestamp(Uptime::Now());
  const auto now = SmallTimestamp::Now();
  const auto after = SmallTimestamp(Uptime::Now());
  EXPECT_LE(before, now);
  EXPECT_LE(now, after);
}

TEST(SmallTimestamp, ModularDifferenceAcrossManyOrigins) {
  uint32_t seed = 17;
  for (int i = 0; i < 10000; ++i) {
    seed = seed * 1664525u + 1013904223u;
    const uint32_t origin = seed;
    seed = seed * 1664525u + 1013904223u;
    int32_t delta = static_cast<int32_t>(seed & INT32_MAX);
    if (i % 2)
      delta = -delta;
    const auto a = At(origin), b = At(origin + static_cast<uint32_t>(delta));
    EXPECT_EQ(delta, (b - a).inMillis());
    EXPECT_EQ(-delta, (a - b).inMillis());
    EXPECT_EQ(delta > 0, b > a);
    EXPECT_EQ(delta < 0, b < a);
  }
}

#ifndef NDEBUG
TEST(CompactTimeDeathTest, RejectsDetectableRangeViolations) {
  EXPECT_DEATH(
      {
        SmallDuration value = SmallSeconds(3000000);
        (void)value;
      },
      "");
  EXPECT_DEATH(
      {
        auto value =
            SmallDuration::Millis(INT32_MAX) + SmallDuration::Millis(1);
        (void)value;
      },
      "");
  EXPECT_DEATH(
      {
        auto value =
            SmallDuration::Millis(INT32_MIN) - SmallDuration::Millis(1);
        (void)value;
      },
      "");
  EXPECT_DEATH(
      {
        auto value = SmallDuration::Millis(INT32_MIN) * -1;
        (void)value;
      },
      "");
  EXPECT_DEATH(
      {
        auto value = At(0) - At(0x80000000u);
        (void)value;
      },
      "");
  EXPECT_DEATH(
      {
        auto value = At(0) < At(0x80000000u);
        (void)value;
      },
      "");
  EXPECT_DEATH(
      {
        auto value = Hours(UINT64_MAX).inMicros();
        (void)value;
      },
      "");
}
#endif
} // namespace
} // namespace roo_time

TEST(CompactTimestampShift, FullDurationsAndUnitExpressions) {
  using namespace roo_time;
  const SmallTimestamp start = Uptime::Start();
  const Duration shift = Seconds(1) + Millis(500);
  EXPECT_EQ(1500, ((start + shift) - start).inMillis());
  EXPECT_EQ(1500, ((shift + start) - start).inMillis());
  EXPECT_EQ(-1500, ((start - shift) - start).inMillis());
  EXPECT_EQ(start, start + Micros(999));
  EXPECT_EQ(-1, ((start + Micros(-1999)) - start).inMillis());
  EXPECT_EQ(5000, ((start + Seconds(5)) - start).inMillis());
  SmallDuration compact = SmallMillis(250);
  EXPECT_EQ(250, ((start + compact) - start).inMillis());
  auto t = start;
  t += shift;
  t -= Seconds(1) + Millis(500);
  EXPECT_EQ(start, t);
#ifndef NDEBUG
  EXPECT_DEATH({ auto bad = start + Seconds(3000000); (void)bad; }, "");
#endif
}
