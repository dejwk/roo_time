#include "roo_time.h"

#if defined(ARDUINO_ARCH_RP2040) && defined(__has_include)
#if __has_include(<pico/time.h>)
#define ROO_TIME_PICO_ARDUINO 1
#endif
#endif

#if defined(ROO_TESTING)

#include "roo_testing/system/timer.h"

inline static int64_t __uptime() { return system_time_get_micros(); }

#define ROO_TIME_UPTIME_MONOTONE 1

inline static void __delayMicros(int64_t micros) {
  system_time_delay_micros(micros);
}

#elif defined(ESP32) || defined(ESP_PLATFORM)

#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if defined(ESP32)
#include <Arduino.h>
inline static void __busyWaitMicros(uint32_t micros) {
  delayMicroseconds(micros);
}
#else
#include <rom/ets_sys.h>
inline static void __busyWaitMicros(uint32_t micros) { ets_delay_us(micros); }
#endif

extern "C" {
int64_t esp_timer_get_time();
}

inline static IRAM_ATTR int64_t __uptime() { return esp_timer_get_time(); }

#define ROO_TIME_UPTIME_MONOTONE 1

inline static void __delayMicros(int64_t micros) {
  // ESP32 uses at most 32-bit ticks. Avoid the indefinite-wait sentinel and
  // perform conversion in 64 bits rather than overflowing pdMS_TO_TICKS.
  constexpr uint64_t kMaxDelayMicros =
      (static_cast<uint64_t>(portMAX_DELAY) - 1) * 1000000 /
      configTICK_RATE_HZ;
  if (static_cast<uint64_t>(micros) > kMaxDelayMicros) {
    micros = kMaxDelayMicros;
  }
  const TickType_t ticks = static_cast<TickType_t>(
      static_cast<uint64_t>(micros) * configTICK_RATE_HZ / 1000000);
  if (micros < 2000 || ticks == 0) {
    __busyWaitMicros(static_cast<uint32_t>(micros));
  } else {
    vTaskDelay(ticks);
  }
}

#elif (defined(PICO_ON_DEVICE) && PICO_ON_DEVICE) || \
    defined(ROO_TIME_PICO_ARDUINO)

#include <pico/time.h>

#define ROO_TIME_UPTIME_MONOTONE 1

inline static int64_t __uptime() {
  return static_cast<int64_t>(time_us_64());
}

inline static void __delayMicros(int64_t micros) {
  sleep_us(static_cast<uint64_t>(micros));
}

#elif defined(ARDUINO)

#include <Arduino.h>

inline static int64_t __uptime() { return micros(); }

inline static void __delayMicros(int64_t micros) {
  // Sample at half the 32-bit counter period, leaving half a period of margin
  // for scheduling delays. The resulting millisecond argument also fits 32 bits.
  constexpr int64_t kMaxDelayMicros = int64_t{1} << 31;
  if (micros > kMaxDelayMicros) micros = kMaxDelayMicros;
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

SmallTimestamp IRAM_ATTR SmallTimestamp::Now() {
  SmallTimestamp result;
#if ROO_TIME_UPTIME_MONOTONE
  // Native counters need no software extension. Keep the uptime clock's origin
  // and sleep accounting, then truncate before retaining the low 32 bits.
  result.millis_ = static_cast<uint32_t>(__uptime() / 1000);
#else
  // Arduino already maintains a millisecond counter. Avoid 64-bit arithmetic
  // and the shared micros() rollover-extension state entirely.
  result.millis_ = static_cast<uint32_t>(millis());
#endif
  return result;
}

void IRAM_ATTR Delay(Duration duration) {
  if (duration.inMicros() <= 0) return;
  const Uptime start = Uptime::Now();
  Duration remaining = duration;
  for (;;) {
    // Each backend bounds its own wait only where required by counter or API
    // limits. Recheck elapsed time even if a coarse wait returned early.
    __delayMicros(remaining.inMicros());
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
