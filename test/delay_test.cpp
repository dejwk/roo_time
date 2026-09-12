#include <limits>
#include "gtest/gtest.h"
#include "roo_time.h"
#include "freertos/FreeRTOS.h"

#undef ROO_TESTING
#undef ESP32
#undef ESP_PLATFORM
#undef ARDUINO
#if defined(TEST_EMULATED_DELAY)
#define ROO_TESTING 1
#elif defined(TEST_GENERIC_ARDUINO_DELAY)
#define ARDUINO 1
#elif defined(TEST_ARDUINO_ESP32_DELAY)
#define ESP32 1
#define ARDUINO 1
#else
#define ESP_PLATFORM 1
#endif
#include "src/uptime_now.cpp"

namespace {
int64_t counter = 100000;
int delay_calls = 0;
int coarse_waits = 0;
uint64_t largest_delay = 0;
void RecordDelay(uint64_t micros) {
  ++delay_calls;
  if (micros > largest_delay) largest_delay = micros;
}
}
// Each clock access models one microsecond of execution, so a zero-tick yield
// still allows time to advance. Coarse waits model waking near a tick boundary.
extern "C" int64_t esp_timer_get_time() { return counter++; }
void vTaskDelay(uint32_t ticks) {
  RecordDelay(ticks * 10000ULL);
  EXPECT_LT(ticks, static_cast<uint64_t>(portMAX_DELAY));
  if (ticks > 0) {
    ++coarse_waits;
    counter += (ticks - 1) * 10000LL + 1;
  }
}
void ets_delay_us(uint32_t micros) {
  RecordDelay(micros);
  counter += micros;
}
uint32_t micros() { return static_cast<uint32_t>(counter++); }
void delay(uint32_t millis) {
  ++coarse_waits;
  RecordDelay(millis * 1000ULL);
  counter += millis * 1000LL;
}
void delayMicroseconds(uint32_t micros) { ets_delay_us(micros); }
int64_t system_time_get_micros() { return counter++; }
void system_time_delay_micros(uint64_t micros) {
  RecordDelay(micros);
  counter += micros;
}

TEST(Delay, NonPositiveDurationsDoNotReachBackend) {
  using namespace roo_time;
  delay_calls = 0;
  Delay(Micros(0));
  Delay(Micros(-1));
  Delay(Micros(std::numeric_limits<int64_t>::min()));
  DelayUntil(Uptime::Start());
  DelayUntil(Uptime::Start() + Micros(std::numeric_limits<int64_t>::min()));
  EXPECT_EQ(0, delay_calls);
}

TEST(Delay, WaitsThroughTickRounding) {
  using namespace roo_time;
  largest_delay = 0;
  for (int64_t micros : {1LL, 1999LL, 2000LL, 5000LL, 10000LL, 11001LL,
                         3000001LL}) {
    const auto start = Uptime::Now();
    Delay(Micros(micros));
    EXPECT_GE(Uptime::Now() - start, Micros(micros));
    const auto deadline = Uptime::Now() + Micros(micros);
    DelayUntil(deadline);
    EXPECT_GE(Uptime::Now(), deadline);
  }
  EXPECT_GE(largest_delay, 3000000u);
}

TEST(Delay, TenMinutesUsesOneLongWait) {
  using namespace roo_time;
  delay_calls = 0;
  coarse_waits = 0;
  largest_delay = 0;
  const auto start = Uptime::Now();
  Delay(Minutes(10));
  EXPECT_GE(Uptime::Now() - start, Minutes(10));
  EXPECT_EQ(largest_delay, 600000000u);
#ifdef TEST_EMULATED_DELAY
  EXPECT_EQ(delay_calls, 1);
#else
  EXPECT_EQ(coarse_waits, 1);
#endif
}

#ifndef TEST_EMULATED_DELAY
TEST(Delay, LongWaitRespectsBackendRange) {
  using namespace roo_time;
  coarse_waits = 0;
  largest_delay = 0;
#ifdef TEST_GENERIC_ARDUINO_DELAY
  // Cross more than one full micros() period while sleeping.
  const auto duration = Hours(2);
  constexpr uint64_t limit = uint64_t{1} << 31;
#else
  constexpr uint64_t limit =
      (static_cast<uint64_t>(portMAX_DELAY) - 1) * 1000000 / configTICK_RATE_HZ;
  const auto duration = Micros(limit + 20000);
#endif
  const auto start = Uptime::Now();
  Delay(duration);
  EXPECT_GE(Uptime::Now() - start, duration);
  EXPECT_GT(coarse_waits, 1);
  EXPECT_LE(largest_delay, limit);
}
#endif
