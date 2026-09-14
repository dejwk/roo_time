#include "roo_time/duration.h"

namespace roo_time {
namespace {

const int64_t kMaxComponentizedDuration =
    (1024LL * 1024 * 64) * 24 * 3600 * 1000000LL - 1;
}

Duration::Components Duration::toComponents() const {
  Duration::Components c;
  c.negative = (micros_ < 0);
  uint64_t v = c.negative ? uint64_t{0} - static_cast<uint64_t>(micros_)
                          : static_cast<uint64_t>(micros_);
  if (v > kMaxComponentizedDuration) v = kMaxComponentizedDuration;
  c.micros = v % 1000000L;
  v /= 1000000L;
  c.seconds = v % 60;
  v /= 60;
  c.minutes = v % 60;
  v /= 60;
  c.hours = v % 24;
  v /= 24;
  c.days = v;
  return c;
}

Duration Duration::FromComponents(const Duration::Components& c) {
  int64_t micros =
      (((static_cast<int64_t>(c.days) * 24 + c.hours) * 60 + c.minutes) * 60 +
       c.seconds) *
          1000000LL +
      c.micros;
  if (c.negative) micros = -micros;
  return Micros(micros);
}

}  // namespace roo_time
