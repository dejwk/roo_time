#pragma once

#include <cstddef>

#include "roo_time/timezone.h"

namespace roo_time {
namespace testing {

/// Borrowed transition table used to exercise the calendar-provider contract.
/// Query cost is O(change_count); test tables have at most two reverse matches.
class SyntheticCalendarZone : public TimeZone {
 public:
  struct Change {
    WallTime instant;
    UtcOffset after;
  };

  SyntheticCalendarZone(TimeRange range, UtcOffset initial,
                        const Change* changes, size_t change_count)
      : range_(range),
        initial_(initial),
        changes_(changes),
        change_count_(change_count) {}

  UtcOffset resolveOffset(WallTime instant) const override {
    UtcOffset offset = initial_;
    for (size_t i = 0; i < change_count_; ++i) {
      if (instant < changes_[i].instant) break;
      offset = changes_[i].after;
    }
    return offset;
  }

  CalendarStatus calendarRange(TimeRange& out) const override {
    out = range_;
    return CalendarStatus::kOk;
  }

  CalendarStatus offsetSpanAt(WallTime instant,
                              OffsetSpan& out) const override {
    if (!instant.isSet()) return CalendarStatus::kInvalidArgument;
    if (instant < range_.begin || instant >= range_.end)
      return CalendarStatus::kOutOfRange;
    out = {range_, initial_};
    for (size_t i = 0; i < change_count_; ++i) {
      if (instant < changes_[i].instant) {
        out.range.end = changes_[i].instant;
        break;
      }
      out.range.begin = changes_[i].instant;
      out.offset = changes_[i].after;
    }
    return CalendarStatus::kOk;
  }

  LocalTimeResolution resolveLocal(CivilDay date,
                                   TimeOfDay time) const override {
    DateTime local;
    if (!DateTime::TryFromComponents(date, time, timezone::UTC, local))
      return {LocalTimeStatus::kInvalidInput};
    LocalTimeResolution result{LocalTimeStatus::kNonexistent};
    WallTime begin = range_.begin;
    UtcOffset offset = initial_;
    for (size_t i = 0; i <= change_count_; ++i) {
      const WallTime end =
          i == change_count_ ? range_.end : changes_[i].instant;
      const WallTime candidate = local.wallTime() - offset.asDuration();
      if (candidate >= begin && candidate < end) {
        if (!result.first.isSet()) {
          result = {LocalTimeStatus::kUnique, candidate};
        } else {
          result.status = LocalTimeStatus::kAmbiguous;
          result.second = candidate;
        }
      }
      if (i != change_count_) {
        begin = end;
        offset = changes_[i].after;
      }
    }
    return result;
  }

 private:
  TimeRange range_;
  UtcOffset initial_;
  const Change* changes_;
  size_t change_count_;
};

}  // namespace testing
}  // namespace roo_time
