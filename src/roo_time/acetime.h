#pragma once

#include <AceTime.h>

#include <cassert>
#include <cstdint>

#include "roo_time/timezone.h"

namespace roo_time {

/// Adapts an AceTime timezone for APIs accepting roo_time::TimeZone.
///
/// Include this optional header only in applications that already use AceTime.
/// It stores AceTime's lightweight TimeZone value by copy; its zone processor
/// must remain valid for the adapter's lifetime. AceTime resolves to seconds,
/// while roo_time offsets are whole minutes, so second-granularity historical
/// offsets assert. AceTime's supported instant range also applies.
class AceTimeZone final : public TimeZone {
 public:
  /// Wraps an AceTime timezone value.
  explicit AceTimeZone(ace_time::TimeZone zone) : zone_(zone) {}

  /// Resolves the total local-minus-UTC offset at instant.
  ///
  /// Asserts if AceTime cannot represent instant, returns an error, or resolves
  /// a sub-minute offset.
  UtcOffset resolveOffset(WallTime instant) const override {
    const int64_t unix_seconds = instant.sinceEpoch().inSecondsFloor();
    const ace_time::OffsetDateTime utc =
        ace_time::OffsetDateTime::forUnixSeconds64(
            unix_seconds, ace_time::TimeOffset::forSeconds(0));
    assert(!utc.isError());
    const ace_time::OffsetDateTime local =
        zone_.getOffsetDateTime(utc.toEpochSeconds());
    assert(!local.isError());
    const int32_t offset_seconds = local.timeOffset().toSeconds();
    assert(offset_seconds % 60 == 0);
    return UtcOffset(Minutes(offset_seconds / 60));
  }

 private:
  ace_time::TimeZone zone_;
};

}  // namespace roo_time
