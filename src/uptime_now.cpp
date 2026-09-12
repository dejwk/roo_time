#include "roo_time.h"

#if defined(ROO_TESTING)

#include "roo_testing/system/timer.h"

inline static int64_t __uptime() { return system_time_get_micros(); }

#define ROO_TIME_UPTIME_MONOTONE 1

inline static void __delayMicros(int64_t micros) {
  system_time_delay_micros(micros);
}

#elif defined(ESP32)

#include <Arduino.h>

#include "esp_attr.h"

extern "C" {
int64_t esp_timer_get_time();
}

inline static IRAM_ATTR int64_t __uptime() { return esp_timer_get_time(); }

#define ROO_TIME_UPTIME_MONOTONE 1

inline static void __delayMicros(int64_t micros) {
  if (micros < 0) {
    return;
  } else if (micros < 2000) {
    delayMicroseconds(micros);
  } else {
    delay(micros / 1000);
    delayMicroseconds(micros % 1000);
  }
}

#elif defined(ESP_PLATFORM)

#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <rom/ets_sys.h>

extern "C" {
int64_t esp_timer_get_time();
}

inline static IRAM_ATTR int64_t __uptime() { return esp_timer_get_time(); }

#define ROO_TIME_UPTIME_MONOTONE 1

inline static void __delayMicros(int64_t micros) {
  if (micros < 0) {
    return;
  } else if (micros < 2000) {
    ets_delay_us(micros);
  } else {
    vTaskDelay(pdMS_TO_TICKS(micros / 1000));
    ets_delay_us(micros % 1000);
  }
}

#elif defined(ARDUINO)

#include <Arduino.h>

inline static int64_t __uptime() { return micros(); }

inline static void __delayMicros(int64_t micros) {
  if (micros < 0) {
    return;
  } else if (micros < 2000) {
    delayMicroseconds(micros);
  } else {
    delay(micros / 1000);
    delayMicroseconds(micros % 1000);
  }
}

#elif defined(__linux__)

#include <chrono>
#include <thread>

#define ROO_TIME_UPTIME_MONOTONE 1

inline static int64_t __uptime() {
  static const auto origin = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(
             now - origin)
      .count();
}

inline static void __delayMicros(int64_t micros) {
  std::this_thread::sleep_for(std::chrono::microseconds(micros));
}

#endif

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

namespace roo_time {

#ifndef ROO_TIME_UPTIME_MONOTONE
#define ROO_TIME_UPTIME_MONOTONE 0
#endif

#if ROO_TIME_UPTIME_MONOTONE

const Uptime IRAM_ATTR Uptime::Now() { return Uptime(__uptime()); }

#else  // Arduino platforms with a wrapping 32-bit microsecond counter.

// Calls through this fallback must be serialized by the application. Sample at
// least once per counter period, including once before its first rollover.
static uint32_t last_reading = 0;
static int64_t elapsed_micros = 0;

const Uptime IRAM_ATTR Uptime::Now() {
  uint32_t now = static_cast<uint32_t>(__uptime());
  elapsed_micros += static_cast<uint32_t>(now - last_reading);
  last_reading = now;
  return Uptime(elapsed_micros);
}

#endif

void IRAM_ATTR Delay(Duration duration) {
  if (duration.inMicros() <= 0) return;
  const Uptime start = Uptime::Now();
  Duration remaining = duration;
  for (;;) {
    // Bound platform argument widths and sample wrapping counters frequently.
    const int64_t chunk = remaining.inMicros() < 1000000
                              ? remaining.inMicros() : 1000000;
    __delayMicros(chunk);
    const Duration elapsed = Uptime::Now() - start;
    if (elapsed >= duration) return;
    remaining = duration - elapsed;
  }
}

void IRAM_ATTR DelayUntil(Uptime deadline) {
  const Uptime now = Uptime::Now();
  // Compare before subtracting, so even the most negative deadline is a no-op.
  if (deadline <= now) return;
  Delay(deadline - now);
}

}  // namespace roo_time
