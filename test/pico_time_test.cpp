#include "gtest/gtest.h"
#include "roo_time.h"
#undef ROO_TESTING
#undef ESP32
#undef ESP_PLATFORM
#undef ARDUINO
#ifdef TEST_PICO_ARDUINO
#define ARDUINO 1
#define ARDUINO_ARCH_RP2040 1
#else
#define PICO_ON_DEVICE 1
#endif
#include "src/uptime_now.cpp"
namespace {
uint64_t counter = 0;
uint64_t waited = 0;
unsigned calls = 0;
}
uint64_t time_us_64() { return counter; }
void sleep_us(uint64_t micros) { counter += micros; waited = micros; ++calls; }
TEST(PicoTime, NativeCounterAndLongSleep) {
  using namespace roo_time;
  counter = 3ULL << 32; // No earlier samples; already past multiple 32-bit wraps.
  EXPECT_EQ(counter, static_cast<uint64_t>(Uptime::Now().inMicros()));
  auto start = Uptime::Now();
  counter += 5ULL << 32;
  EXPECT_EQ(5LL << 32, (Uptime::Now() - start).inMicros());
  const auto deadline = Uptime::Now() + Hours(2);
  calls = 0;
  DelayUntil(deadline);
  EXPECT_EQ(1u, calls);
  EXPECT_EQ(7200000000ULL, waited);
  EXPECT_EQ(deadline, Uptime::Now());
  Delay(Micros(0));
  Delay(Micros(-1));
  DelayUntil(deadline);
  EXPECT_EQ(1u, calls);
}

TEST(PicoTime, CompactClockTruncatesBeforeWrapping) {
  using namespace roo_time;
  counter = 0xffffffffULL * 1000 + 999;
  auto last = SmallTimestamp::Now();
  EXPECT_EQ(SmallTimestamp(Uptime::Now()), last);
  counter = 0x100000000ULL * 1000;
  EXPECT_EQ(SmallTimestamp(), SmallTimestamp::Now());
  EXPECT_EQ(1, (SmallTimestamp::Now() - last).inMillis());
}
