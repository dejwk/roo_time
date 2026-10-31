#include <Arduino.h>

#include <cstring>

#include "roo_time.h"
#include "roo_time/format.h"

using namespace roo_time;

struct ScheduleSettings {
  CivilDay first_day = CivilDay::FromYmd(2026, 6, 15);
  TimeOfDay start = TimeOfDay::FromHms(6, 30);
};

// Commit both fields together only after the full request validates.
bool ImportSettings(const char* date_text, size_t date_length,
                    const char* time_text, size_t time_length,
                    ScheduleSettings& settings) {
  CivilDay date;
  TimeOfDay time;
  if (ParseCivilDay(date_text, date_length, "%F", &date).status !=
          TextStatus::kOk ||
      ParseTimeOfDay(time_text, time_length, "%H:%M", &time).status !=
          TextStatus::kOk) {
    return false;
  }
  settings.first_day = date;
  settings.start = time;
  return true;
}

bool ExportSettings(const ScheduleSettings& settings) {
  char date[11];
  char time[6];
  if (FormatCivilDay(settings.first_day, "%F", date, sizeof(date)).status !=
          TextStatus::kOk ||
      FormatTimeOfDay(settings.start, "%H:%M", time, sizeof(time)).status !=
          TextStatus::kOk) {
    return false;
  }
  Serial.printf("first_day=%s start=%s\n", date, time);
  return true;
}

void setup() {
  Serial.begin(115200);
  ScheduleSettings settings;
  if (ImportSettings("2026-10-25", 10, "02:30", 5, settings)) {
    ExportSettings(settings);
  }
  if (!ImportSettings("2026-02-30", 10, "08:00", 5, settings)) {
    Serial.println("Invalid date; saved settings retained.");
    ExportSettings(settings);
  }
}

void loop() { Delay(Seconds(1)); }
