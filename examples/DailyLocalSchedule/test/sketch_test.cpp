#include <gtest/gtest.h>

#include "DailyLocalSchedule.ino"

// Verifies the schedule skips the spring gap and selects the first autumn run.
TEST(DailyLocalSchedule, ExplicitTransitionPolicy) {
  WallTime run = WallTime::Epoch();
  EXPECT_TRUE(PlanDay(CivilDay::FromYmd(2026, 3, 29), TimeOfDay::FromHms(2, 30),
                      schedule_zone, run));
  EXPECT_FALSE(run.isSet());
  EXPECT_TRUE(PlanDay(CivilDay::FromYmd(2026, 10, 25),
                      TimeOfDay::FromHms(2, 30), schedule_zone, run));
  EXPECT_EQ(run, DateTime(2026, 10, 25, 0, 30, 0, 0, timezone::UTC).wallTime());
  EXPECT_FALSE(
      PlanDay(CivilDay(), TimeOfDay::FromHms(2, 30), schedule_zone, run));
  EXPECT_FALSE(run.isSet());
}

// Verifies advancing calendar dates preserves local time across a 23-hour day.
TEST(DailyLocalSchedule, CalendarDayIsNot24Hours) {
  WallTime before = WallTime::Unset();
  WallTime after = WallTime::Unset();
  const CivilDay first = CivilDay::FromYmd(2026, 3, 28);
  ASSERT_TRUE(PlanDay(first, TimeOfDay::FromHms(6, 30), schedule_zone, before));
  ASSERT_TRUE(PlanDay(first.addDays(1), TimeOfDay::FromHms(6, 30),
                      schedule_zone, after));
  EXPECT_EQ(after - before, Hours(23));
}

// Verifies the actual sketch emits both the gap and the chosen overlap instant.
TEST(DailyLocalSchedule, ScriptedCalendar) {
  testing::internal::CaptureStdout();
  setup();
  const std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("2026-03-29: skipped"), std::string::npos);
  EXPECT_NE(
      output.find("2026-10-25 02:30 local -> 2026-10-25T00:30:00.000000+00:00"),
      std::string::npos);
}
