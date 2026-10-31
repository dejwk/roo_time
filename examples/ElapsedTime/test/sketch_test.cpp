#include <gtest/gtest.h>

#include "ElapsedTime.ino"

// Verifies the actual sketch prints separate uptime and elapsed-duration
// fields.
TEST(ElapsedTime, OneRead) {
  testing::internal::CaptureStdout();
  setup();
  loop();
  const std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("S+"), std::string::npos);
  EXPECT_NE(output.find("sensor read took 00:00:"), std::string::npos);
}
