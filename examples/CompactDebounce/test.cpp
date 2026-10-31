#include <gtest/gtest.h>

#include "CompactDebounce.ino"

// Verifies noise is ignored and the threshold remains correct across rollover.
TEST(CompactDebounce, BounceAndRollover) {
  DebouncedButton button;
  const Uptime origin = Uptime::Start() + Millis(0xFFFFFFF0LL);
  EXPECT_FALSE(button.update(true, SmallTimestamp(origin)));
  EXPECT_FALSE(button.update(false, SmallTimestamp(origin + Millis(2))));
  EXPECT_FALSE(button.update(true, SmallTimestamp(origin + Millis(4))));
  EXPECT_FALSE(button.update(true, SmallTimestamp(origin + Millis(23))));
  EXPECT_TRUE(button.update(true, SmallTimestamp(origin + Millis(24))));
  EXPECT_TRUE(button.pressed());
}

// Verifies the replay debounces two independent inputs and prints their events.
TEST(CompactDebounce, ScriptedInputs) {
  testing::internal::CaptureStdout();
  setup();
  const std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("23 ms: button 1 pressed"), std::string::npos);
  EXPECT_NE(output.find("25 ms: button 0 pressed"), std::string::npos);
  EXPECT_NE(output.find("50 ms: button 0 released"), std::string::npos);
}
