#include <gtest/gtest.h>

#include "Esp32NtpTime.ino"
#include "roo_testing/frameworks/esp32_shims/sntp_host.h"
#include "roo_testing/system/wall_clock.h"

namespace {
namespace host = roo_testing::esp32::sntp;
class Esp32NtpTime : public testing::Test {
 protected:
  void SetUp() override {
    host::Reset();
    roo_testing::system::ResetToPowerOnMapping();
    time_ready = false;
  }
  void TearDown() override {
    host::Reset();
    roo_testing::system::ResetToPowerOnMapping();
  }
};

// Verifies a readable cold clock is distinct from successful SNTP notification.
TEST_F(Esp32NtpTime, ColdClockThenServiceNotification) {
  const WallTime cold = system_clock.now();
  ASSERT_TRUE(cold.isSet());
  EXPECT_EQ(1970, DateTime(cold, timezone::UTC).year());
  EXPECT_FALSE(WaitForSync(Millis(0)));
  host::ServerBehavior server;
  server.time_source = host::TimeSource::kReference;
  server.reference_unix_us = 1700000000000000;
  server.response_delay_us = 1000;
  ASSERT_EQ(ESP_OK, host::ConfigureServer(server));
  StartClock();
  ASSERT_TRUE(WaitForSync(Seconds(1)));
  EXPECT_GE(system_clock.now().sinceEpoch().inMicros(),
            server.reference_unix_us);
}

// Verifies a silent virtual server cannot make uptime/read validity imply sync.
TEST_F(Esp32NtpTime, SilentServer) {
  host::ServerBehavior server;
  server.responds = false;
  server.response_delay_us = 1000;
  ASSERT_EQ(ESP_OK, host::ConfigureServer(server));
  StartClock();
  EXPECT_FALSE(WaitForSync(Millis(20)));
  EXPECT_EQ(1970, DateTime(system_clock.now(), timezone::UTC).year());
}

// Verifies actual Wi-Fi setup and both displays use the emulator SNTP service.
TEST_F(Esp32NtpTime, VirtualServerSmoke) {
  testing::internal::CaptureStdout();
  setup();
  loop();
  const std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("SNTP synchronization confirmed."), std::string::npos);
  EXPECT_NE(output.find("UTC: "), std::string::npos);
  EXPECT_NE(output.find("Local: "), std::string::npos);
}
}  // namespace
