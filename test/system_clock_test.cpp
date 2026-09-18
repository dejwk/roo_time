#include "gtest/gtest.h"
#include "roo_time.h"

namespace {
int clock_result = 0;
struct timeval clock_value = {};
}  // namespace

// Supplies deterministic clock successes and failures without changing host
// time.
extern "C" int __wrap_gettimeofday(struct timeval* value, void*) {
  if (clock_result == 0) *value = clock_value;
  return clock_result;
}

// Verifies read errors are distinguishable from a successful epoch reading.
TEST(SystemClock, ReadErrorAndRecovery) {
  const roo_time::SystemClock clock;
  clock_result = -1;
  EXPECT_FALSE(clock.now().isSet());
  clock_result = 0;
  clock_value = {};
  EXPECT_TRUE(clock.now().isSet());
  EXPECT_EQ(roo_time::Micros(0), clock.now().sinceEpoch());
  clock_value.tv_sec = -1;
  clock_value.tv_usec = 123456;
  EXPECT_EQ(roo_time::Micros(-876544), clock.now().sinceEpoch());
  clock_value.tv_sec = 1700000000;
  EXPECT_EQ(roo_time::Micros(1700000000123456LL), clock.now().sinceEpoch());
}
