#include <gtest/gtest.h>

#include "ScheduleSettings.ino"

// Verifies a bad second field cannot partially replace valid saved settings.
TEST(ScheduleSettings, AtomicImport) {
  ScheduleSettings settings;
  const CivilDay original = settings.first_day;
  const TimeOfDay original_time = settings.start;
  EXPECT_FALSE(ImportSettings("2026-10-25", 10, "24:00", 5, settings));
  EXPECT_EQ(settings.first_day, original);
  EXPECT_EQ(settings.start, original_time);
  EXPECT_FALSE(ImportSettings("2026-02-30", 10, "08:00", 5, settings));
  EXPECT_EQ(settings.first_day, original);
  EXPECT_FALSE(ImportSettings("2026-10-25junk", 14, "08:00", 5, settings));
  EXPECT_TRUE(ImportSettings("2026-10-25", 10, "02:30", 5, settings));
  EXPECT_EQ(settings.first_day, CivilDay::FromYmd(2026, 10, 25));
  EXPECT_EQ(settings.start, TimeOfDay::FromHms(2, 30));
}

// Verifies the demonstration exports a valid edit and retains it after an
// error.
TEST(ScheduleSettings, RoundTrip) {
  testing::internal::CaptureStdout();
  setup();
  const std::string output = testing::internal::GetCapturedStdout();
  const std::string expected = "first_day=2026-10-25 start=02:30";
  const size_t first = output.find(expected);
  ASSERT_NE(first, std::string::npos);
  EXPECT_NE(output.find(expected, first + expected.size()), std::string::npos);
  EXPECT_NE(output.find("Invalid date"), std::string::npos);
}
