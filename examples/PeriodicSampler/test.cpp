#include <gtest/gtest.h>

#include "PeriodicSampler.ino"

// Verifies on-time and late work preserve the original deadline grid.
TEST(PeriodicSampler, MissedDeadlines) {
  Uptime next = Uptime::Start() + Millis(100);
  EXPECT_EQ(SkipMissedSamples(Uptime::Start() + Millis(99), next), 0);
  EXPECT_EQ(SkipMissedSamples(Uptime::Start() + Millis(275), next), 2);
  EXPECT_EQ(next, Uptime::Start() + Millis(300));
  EXPECT_EQ(SkipMissedSamples(next, next), 1);
  EXPECT_EQ(next, Uptime::Start() + Millis(400));
}

// Verifies the sketch reports overruns instead of issuing catch-up samples.
TEST(PeriodicSampler, BoundedRun) {
  testing::internal::CaptureStdout();
  setup();
  loop();
  loop();
  const std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("sample 2"), std::string::npos);
  EXPECT_NE(output.find("skipped "), std::string::npos);
}
