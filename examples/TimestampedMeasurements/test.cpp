#include <gtest/gtest.h>

#include "TimestampedMeasurements.ino"

// Verifies unavailable wall time produces a labeled fallback before UTC is
// ready.
TEST(TimestampedMeasurements, ClockBecomesReady) {
  testing::internal::CaptureStdout();
  setup();
  const std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("source=uptime timestamp=S+00:00:00.250000 value=420"),
            std::string::npos);
  EXPECT_NE(
      output.find(
          "source=utc timestamp=2026-06-15T12:30:00.123456+00:00 value=421"),
      std::string::npos);
}

// Verifies insufficient capacity is detected before a truncated record is used.
TEST(TimestampedMeasurements, TimestampBufferTooSmall) {
  char text[8] = {};
  const FormatResult result = FormatIsoDateTime(DateTime(), text, sizeof(text));
  EXPECT_EQ(result.status, TextStatus::kBufferTooSmall);
  EXPECT_GT(result.size, sizeof(text));
}
