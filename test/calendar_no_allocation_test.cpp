#include <cstdlib>
#include <new>

#include "roo_time/calendar_interval.h"
#include "test/synthetic_calendar_zone.h"

namespace {
bool allocation_forbidden = false;
}

extern "C" void* __real_malloc(size_t size);
extern "C" void* __real_calloc(size_t count, size_t size);
extern "C" void* __real_realloc(void* ptr, size_t size);

extern "C" void* __wrap_malloc(size_t size) {
  if (allocation_forbidden) std::abort();
  return __real_malloc(size);
}

extern "C" void* __wrap_calloc(size_t count, size_t size) {
  if (allocation_forbidden) std::abort();
  return __real_calloc(count, size);
}

extern "C" void* __wrap_realloc(void* ptr, size_t size) {
  if (allocation_forbidden) std::abort();
  return __real_realloc(ptr, size);
}

void* operator new(size_t size) {
  void* ptr = std::malloc(size);
  if (ptr == nullptr) std::abort();
  return ptr;
}

void* operator new[](size_t size) { return ::operator new(size); }

void operator delete(void* ptr) noexcept { std::free(ptr); }

void operator delete[](void* ptr) noexcept { std::free(ptr); }

void operator delete(void* ptr, size_t) noexcept { std::free(ptr); }

void operator delete[](void* ptr, size_t) noexcept { std::free(ptr); }

namespace roo_time {
namespace {

struct Accumulator {
  WallTime previous = WallTime::Unset();
  unsigned count = 0;
  bool ordered = true;
};

bool Accumulate(void* context, const CalendarBoundary& boundary) {
  Accumulator& result = *static_cast<Accumulator*>(context);
  if (result.previous.isSet() && result.previous >= boundary.instant)
    result.ordered = false;
  result.previous = boundary.instant;
  ++result.count;
  return true;
}

// Traverses all interval units under a heap-allocation trap, including built-in
// rule calculations, synthetic transitions and fine-grained folded readings.
bool CheckZone(const TimeZone& zone, TimeRange range) {
  const CalendarInterval intervals[] = {
      {CalendarUnit::kMillisecond, 100}, {CalendarUnit::kSecond, 15},
      {CalendarUnit::kMinute, 15},       {CalendarUnit::kHour, 1},
      CalendarInterval::Days(),          CalendarInterval::Weeks(kSunday),
      {CalendarUnit::kMonth, 1},         {CalendarUnit::kYear, 1}};
  for (CalendarInterval interval : intervals) {
    Accumulator result;
    uint32_t work = 4096;
    TimeRange request = range;
    if (interval.unit == CalendarUnit::kMillisecond ||
        interval.unit == CalendarUnit::kSecond)
      request.end = request.begin + Seconds(5);
    if (EnumerateCalendarBoundaries(request, zone, interval, work, Accumulate,
                                    &result) != CalendarStatus::kOk ||
        !result.ordered)
      return false;
  }
  return true;
}

}  // namespace
}  // namespace roo_time

int main() {
  using namespace roo_time;
  allocation_forbidden = true;
  const WallTime begin = DateTime(2026, 10, 25, timezone::UTC).wallTime();
  const TimeRange range{begin, begin + Hours(48)};
  FixedTimeZone fixed(UtcOffset(Minutes(345)));
  RecurringTimeZone recurring(UtcOffset(Hours(1)), rules::EuSummerTime());
  const testing::SyntheticCalendarZone::Change changes[] = {
      {begin + Minutes(7), UtcOffset(Minutes(30))},
      {begin + Minutes(73), timezone::UTC},
      {begin + Minutes(185), UtcOffset(Minutes(-30))},
      {begin + Minutes(287), timezone::UTC}};
  testing::SyntheticCalendarZone synthetic(range, timezone::UTC, changes, 4);
  if (!CheckZone(fixed, range) || !CheckZone(recurring, range) ||
      !CheckZone(synthetic, range))
    return 1;
  return 0;
}
