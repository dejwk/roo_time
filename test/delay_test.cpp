#include <limits>
#include "gtest/gtest.h"
#include "roo_time.h"

#undef ROO_TESTING
#undef ESP32
#undef ESP_PLATFORM
#undef ARDUINO
#if defined(TEST_EMULATED_DELAY)
#define ROO_TESTING 1
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
  if (ticks > 0) counter += (ticks - 1) * 10000 + 1;
}
void ets_delay_us(uint32_t micros) {
  RecordDelay(micros);
  counter += micros;
}
void delay(uint32_t millis) { vTaskDelay(millis / 10); }
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

TEST(Delay, WaitsThroughTickRoundingAndBoundsBackendArguments) {
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
  EXPECT_LE(largest_delay, 1000000u);
}
