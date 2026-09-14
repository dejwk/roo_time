#pragma once

// Optional adapter: the core roo_time.h header does not include <chrono>.
#include "roo_time/duration.h"

// Header presence detects standard duration support, not platform clock
// support. Define to 0 to disable, or to 1 on toolchains without __has_include
// whose standard library is known to provide <chrono>.
#ifndef ROO_TIME_HAS_CHRONO
#if defined(__has_include)
#if __has_include(<chrono>)
#define ROO_TIME_HAS_CHRONO 1
#endif
#endif
#endif
#ifndef ROO_TIME_HAS_CHRONO
#define ROO_TIME_HAS_CHRONO 0
#endif

#if ROO_TIME_HAS_CHRONO
#include <chrono>

namespace roo_time {

/// Converts a roo_time duration to a standard duration (microseconds by
/// default). Accepts SmallDuration through lossless widening to Duration.
/// Integer destination units truncate toward zero. All duration_cast
/// intermediate and final values must be representable; no saturation or range
/// checks occur.
template <typename ChronoDuration = std::chrono::microseconds>
constexpr ChronoDuration ToChrono(Duration value) {
  return std::chrono::duration_cast<ChronoDuration>(
      std::chrono::microseconds(value.inMicros()));
}

/// Converts a standard duration to Duration, truncating fractional microseconds
/// toward zero. Floating values must be finite. All duration_cast intermediate
/// and final values must be representable. Compact storage requires a
/// subsequent explicit SmallDuration conversion. No time-point/clock conversion
/// is implied.
template <typename Rep, typename Period>
constexpr Duration FromChrono(std::chrono::duration<Rep, Period> value) {
  return Micros(
      std::chrono::duration_cast<std::chrono::microseconds>(value).count());
}

}  // namespace roo_time
#endif
