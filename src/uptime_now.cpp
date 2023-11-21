#include "roo_time.h"

#if defined(ESP32) || defined(ESP8266)
#include "esp_attr.h"

extern "C" {
int64_t esp_timer_get_time();
}

inline static IRAM_ATTR int64_t __uptime() { return esp_timer_get_time(); }

#elif defined(ARDUINO) || defined(ROO_TESTING)

#include <Arduino.h>

inline static int64_t __uptime() { return micros(); }

#elif defined(__linux__)
#include <chrono>

inline static int64_t __uptime() {
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(
      now.time_since_epoch()).count();
}
#endif

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

namespace roo_time {

static int64_t last_reading = 0;

// Offset to make sure the time is monotone.
static int64_t offset = 0;

const Uptime IRAM_ATTR Uptime::Now() {
  int64_t now = __uptime() + offset;
  int64_t diff = last_reading - now;
  if (diff > 0) {
    offset += diff;
    now += diff;
  }
  last_reading = now;
  return Uptime(now);
}

}  // namespace roo_time
