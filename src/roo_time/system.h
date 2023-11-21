#pragma once

#include "roo_time.h"
#include "time.h"

namespace roo_time {

// Clock implementation that uses time() as a time source.
class SystemClock : public roo_time::WallTimeClock {
 public:
  SystemClock() {}

  // Returns the current time, using the time() system call.
  WallTime now() const override {
    time_t now;
    time(&now);
    return WallTime(Seconds(now));
  }
};

}  // namespace roo_time