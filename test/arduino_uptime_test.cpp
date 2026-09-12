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
}
uint32_t micros() { return counter; }
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
