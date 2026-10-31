#include <gtest/gtest.h>

#include "Esp32NtpTime.ino"

// Verifies timeout and callback readiness independently of system clock
// validity.
TEST(Esp32NtpTime, SynchronizationReadiness) {
  time_ready = false;
  EXPECT_FALSE(WaitForSync(Millis(0)));
  OnTimeSync(nullptr);
  EXPECT_TRUE(WaitForSync(Millis(0)));
}

// Verifies host Wi-Fi setup and both time displays without claiming real NTP.
TEST(Esp32NtpTime, HostClockSmoke) {
  testing::internal::CaptureStdout();
  setup();
  loop();
  const std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("Host clock ready (simulated sync; no NTP exchange)"),
            std::string::npos);
  EXPECT_NE(output.find("UTC: "), std::string::npos);
  EXPECT_NE(output.find("Local: "), std::string::npos);
}
