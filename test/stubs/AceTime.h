#pragma once

#include <cstdint>

namespace ace_time {

using acetime_t = int32_t;

class TimeOffset {
 public:
  static TimeOffset forSeconds(int32_t seconds) { return TimeOffset(seconds); }

  int32_t toSeconds() const { return seconds_; }

 private:
  explicit TimeOffset(int32_t seconds) : seconds_(seconds) {}

  int32_t seconds_;
};

class OffsetDateTime {
 public:
  static OffsetDateTime forUnixSeconds64(int64_t seconds, TimeOffset) {
    return OffsetDateTime(seconds >= INT32_MIN && seconds <= INT32_MAX,
                          static_cast<acetime_t>(seconds),
                          TimeOffset::forSeconds(0));
  }

  bool isError() const { return !valid_; }

  acetime_t toEpochSeconds() const { return seconds_; }

  TimeOffset timeOffset() const { return offset_; }

  OffsetDateTime(bool valid, acetime_t seconds, TimeOffset offset)
      : valid_(valid), seconds_(seconds), offset_(offset) {}

 private:
  bool valid_;
  acetime_t seconds_;
  TimeOffset offset_;
};

class TimeZone {
 public:
  explicit TimeZone(int32_t offset_seconds) : offset_seconds_(offset_seconds) {}

  OffsetDateTime getOffsetDateTime(acetime_t seconds) const {
    return OffsetDateTime(true, seconds,
                          TimeOffset::forSeconds(offset_seconds_));
  }

 private:
  int32_t offset_seconds_;
};

}  // namespace ace_time
