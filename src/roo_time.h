#pragma once

/// Compatibility facade for roo_time's duration, uptime, and wall-time APIs.

#include "roo_time/duration.h"
#include "roo_time/uptime.h"
#include "roo_time/wall_time.h"

#if defined(__linux__)

/// Convenience printers to aid testing.
#include <iomanip>
#include <ostream>

/// Streams textual `Duration` representation for tests.
inline std::ostream& operator<<(std::ostream& os,
                                const roo_time::Duration& duration) {
  os << duration.inMicros() << " us";
  return os;
}

/// Streams textual `Uptime` representation for tests.
inline std::ostream& operator<<(std::ostream& os, const roo_time::Uptime& t) {
  os << (t - roo_time::Uptime::Start()) << " uptime";
  return os;
}

/// Streams textual `WallTime` representation for tests.
inline std::ostream& operator<<(std::ostream& os, const roo_time::WallTime& t) {
  os << t.sinceEpoch() << " since Epoch";
  return os;
}

/// Streams textual `DateTime` representation for tests.
inline std::ostream& operator<<(std::ostream& os,
                                const roo_time::DateTime& dt) {
  os << std::setfill('0') << std::setw(4) << (int)dt.year() << "-";
  os << std::setfill('0') << std::setw(2) << (int)dt.month() << "-";
  os << std::setfill('0') << std::setw(2) << (int)dt.day() << " ";
  os << std::setfill('0') << std::setw(2) << (int)dt.hour() << ":";
  os << std::setfill('0') << std::setw(2) << (int)dt.minute() << ":";
  os << std::setfill('0') << std::setw(2) << (int)dt.second() << ".";
  os << std::setfill('0') << std::setw(6) << (int64_t)dt.micros();
  if (dt.timeZone().offset().inMicros() > 0) {
    os << "+";
  }
  os << dt.timeZone().offset().inMinutes() << "min";
  return os;
}

#endif
