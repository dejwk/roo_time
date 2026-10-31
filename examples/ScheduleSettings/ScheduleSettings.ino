// Import and export a date and local start time as device settings.
//
// CivilDay holds a calendar date; TimeOfDay holds a clock reading. Neither
// chooses a timezone or identifies a UTC instant. They are natural values for
// settings such as "start on October 25 at 02:30". Resolving that setting into
// an instant is shown separately in DailyLocalSchedule.
//
// This sketch accepts one valid edit, rejects an impossible date, and shows
// that the valid settings remain intact. It uses fixed input text so it runs
// on a board or in the emulator without a UI. Open Serial at 115200 baud.

#include <Arduino.h>

#include "roo_time.h"
#include "roo_time/format.h"

using namespace roo_time;

// Checked factories make these initial settings valid without reading a clock.
struct ScheduleSettings {
  CivilDay first_day = CivilDay::FromYmd(2026, 6, 15);
  TimeOfDay start = TimeOfDay::FromHms(6, 30);
};

// Commit both fields together only after the full request validates.
bool ImportSettings(const char* date_text, size_t date_length,
                    const char* time_text, size_t time_length,
                    ScheduleSettings& settings) {
  // Parse into temporary values. Default-constructed CivilDay and TimeOfDay
  // are invalid; successful parsing replaces them with checked values.
  CivilDay date;
  TimeOfDay time;

  // %F means YYYY-MM-DD for calendar text; %H:%M means a 24-hour clock.
  // Explicit lengths also accept input from bounded request buffers without
  // a terminating NUL. Parsing rejects invalid fields and trailing input.
  if (ParseCivilDay(date_text, date_length, "%F", &date).status !=
          TextStatus::kOk ||
      ParseTimeOfDay(time_text, time_length, "%H:%M", &time).status !=
          TextStatus::kOk) {
    return false;
  }

  // Both fields validated. Only now replace the saved settings, so a bad
  // second field cannot accidentally leave the first field partly updated.
  settings.first_day = date;
  settings.start = time;
  return true;
}

bool ExportSettings(const ScheduleSettings& settings) {
  // Buffer formatting avoids allocation. Reserve space for the terminating
  // NUL as well as the 10 date characters or 5 clock characters.
  char date[11];
  char time[6];
  if (FormatCivilDay(settings.first_day, "%F", date, sizeof(date)).status !=
          TextStatus::kOk ||
      FormatTimeOfDay(settings.start, "%H:%M", time, sizeof(time)).status !=
          TextStatus::kOk) {
    return false;
  }

  // Print only after both formatters report kOk: a too-small buffer otherwise
  // contains a truncated prefix rather than a complete settings value.
  Serial.printf("first_day=%s start=%s\n", date, time);
  return true;
}

void setup() {
  Serial.begin(115200);
  ScheduleSettings settings;

  // Pretend this edit arrived from a settings form or configuration file.
  if (ImportSettings("2026-10-25", 10, "02:30", 5, settings)) {
    ExportSettings(settings);
  }

  // February 30 is rejected, not normalized into a date in March. Even though
  // 08:00 is valid on its own, neither saved field changes after this edit.
  if (!ImportSettings("2026-02-30", 10, "08:00", 5, settings)) {
    Serial.println("Invalid date; saved settings retained.");
    ExportSettings(settings);
  }
}

void loop() {
  // The import/export demonstration runs once in setup().
  Delay(Seconds(1));
}
