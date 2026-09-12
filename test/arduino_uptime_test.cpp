// Exercise the actual Arduino fallback, even under an ESP32 emulator profile.
#include "gtest/gtest.h"
#include "roo_time.h"

#undef ROO_TESTING
#undef ESP32
#undef ESP_PLATFORM
#ifndef ARDUINO
#define ARDUINO 1
#endif
#include "src/uptime_now.cpp"

namespace {
uint32_t counter = 0;
uint32_t millis_counter = 0;
unsigned micros_calls = 0;
}
uint32_t micros() { ++micros_calls; return counter; }
uint32_t millis() { return millis_counter; }
void delay(uint32_t millis) { counter += millis * 1000; }
void delayMicroseconds(uint32_t micros) { counter += micros; }

TEST(ArduinoUptime, PreservesElapsedTimeAcrossSuccessiveRollovers) {
  counter = 0xfffffff0u;
  auto a = roo_time::Uptime::Now();
  EXPECT_EQ(0xfffffff0LL, a.inMicros());
  counter = 0x10u;
  auto b = roo_time::Uptime::Now();
  EXPECT_EQ(32, (b - a).inMicros());
  counter = 0xfffffff0u;
  auto c = roo_time::Uptime::Now();
  EXPECT_EQ(0xffffffe0LL, (c - b).inMicros());
  counter = 0x20u;
  auto d = roo_time::Uptime::Now();
  EXPECT_EQ(48, (d - c).inMicros());
  EXPECT_EQ(0x200000020LL, d.inMicros());
}

TEST(ArduinoSmallTimestamp, UsesMillisWithoutSamplingOrMutatingUptime) {
  using namespace roo_time;
  // No prior compact samples: the micros clock has wrapped many times.
  const auto calls_before = micros_calls;
  const auto extension_before = elapsed_micros;
  const auto reading_before = last_reading;
  millis_counter = 0xfffffff0u;
  auto start = SmallTimestamp::Now();
  EXPECT_EQ(SmallTimestamp(Uptime::Start() + Millis(millis_counter)), start);
  millis_counter = 0x10u;
  EXPECT_EQ(32, (SmallTimestamp::Now() - start).inMillis());
  EXPECT_EQ(calls_before, micros_calls);
  EXPECT_EQ(extension_before, elapsed_micros);
  EXPECT_EQ(reading_before, last_reading);
}
