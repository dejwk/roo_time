#include <Arduino.h>

#include "roo_time.h"
#include "roo_time/format.h"
#include "roo_time/timezone.h"

using namespace roo_time;

const RecurringTimeZone schedule_zone(UtcOffset(Hours(1)),
                                      rules::EuSummerTime());

// Application policy: run once at the earlier overlap occurrence; skip gaps.
// Unsupported zones and invalid/out-of-range values are errors, not skips.
bool PlanDay(CivilDay date, TimeOfDay time, const TimeZone& zone,
             WallTime& occurrence) {
  occurrence = WallTime::Unset();
  const LocalTimeResolution result = ResolveLocal(date, time, zone);
  switch (result.status) {
    case LocalTimeStatus::kUnique:
    case LocalTimeStatus::kAmbiguous:
      occurrence = result.first;
      return true;
    case LocalTimeStatus::kNonexistent:
      return true;
    default:
      return false;
  }
}

// Plan calendar days, rather than repeatedly adding a 24-hour duration.
void PrintPlan(CivilDay first_day) {
  for (int i = 0; i < 3; ++i) {
    const CivilDay date = first_day.addDays(i);
    char day[11];
    if (FormatCivilDay(date, "%F", day, sizeof(day)).status != TextStatus::kOk)
      return;
    WallTime occurrence = WallTime::Unset();
    if (!PlanDay(date, TimeOfDay::FromHms(2, 30), schedule_zone, occurrence)) {
      Serial.printf("%s: resolution error\n", day);
    } else if (!occurrence.isSet()) {
      Serial.printf("%s: skipped (clock jumps over 02:30)\n", day);
    } else {
      char utc[40];
      if (FormatIsoDateTime(DateTime(occurrence, timezone::UTC), utc,
                            sizeof(utc))
              .status == TextStatus::kOk) {
        Serial.printf("%s 02:30 local -> %s\n", day, utc);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  PrintPlan(CivilDay::FromYmd(2026, 3, 28));
  PrintPlan(CivilDay::FromYmd(2026, 10, 24));
}

void loop() { Delay(Seconds(1)); }
