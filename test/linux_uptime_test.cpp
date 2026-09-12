#include <chrono>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "roo_time.h"

#undef ROO_TESTING
#undef ESP32
#undef ESP_PLATFORM
#undef ARDUINO
#include "src/uptime_now.cpp"

TEST(LinuxUptime, RelativeMonotonicClockAndConcurrentReaders) {
  using namespace roo_time;
  auto start = Uptime::Now();
  EXPECT_GE(start, Uptime::Start());
  EXPECT_LT(start - Uptime::Start(), Seconds(5));
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  EXPECT_GE(Uptime::Now() - start, Millis(2));
  std::vector<std::thread> readers;
  for (int i = 0; i < 4; ++i) {
    readers.emplace_back([] {
      auto previous = Uptime::Now();
      for (int j = 0; j < 1000; ++j) {
        auto now = Uptime::Now();
        EXPECT_GE(now, previous);
        previous = now;
      }
    });
  }
  for (auto& reader : readers) reader.join();
}
