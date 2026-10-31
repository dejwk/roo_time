// Plan an operation for 02:30 local time on successive calendar dates.
//
// Local clock readings do not always identify one instant: a spring clock
// change can skip 02:30, while an autumn change can repeat it. ResolveLocal
// reports these cases so the application can choose its own scheduling policy.
// Here we skip missing times and run once at the earlier repeated occurrence.
//
// The sketch prints three dates around each 2026 EU clock change. It plans
// occurrences without waiting for those dates, so it runs immediately on a
// board or in the emulator. Open Serial at 115200 baud. This demonstrates
// calendar planning; it does not execute or persist a running daily schedule.

#include <Arduino.h>

#include "roo_time.h"
#include "roo_time/format.h"
#include "roo_time/timezone.h"

using namespace roo_time;

// Select a winter offset of UTC+1 and the recurring EU summer-time pattern.
// This describes an annual pattern, not a geographic historical zone database.
const RecurringTimeZone schedule_zone(UtcOffset(Hours(1)),
                                      rules::EuSummerTime());

// Application policy: run once at the earlier overlap occurrence; skip gaps.
// Unsupported zones and invalid/out-of-range values are errors, not skips.
bool PlanDay(CivilDay date, TimeOfDay time, const TimeZone& zone,
             WallTime& occurrence) {
  occurrence = WallTime::Unset();

  // A civil date plus a time of day needs a zone before it can become an
  // instant. The result distinguishes one occurrence, two occurrences, and
  // gaps.
  const LocalTimeResolution result = ResolveLocal(date, time, zone);
  switch (result.status) {
    case LocalTimeStatus::kUnique:
    case LocalTimeStatus::kAmbiguous:
      // first is the unique instant or the earlier of two repeated instants.
      // Selecting second instead would implement the later-occurrence policy.
      occurrence = result.first;
      return true;
    case LocalTimeStatus::kNonexistent:
      // A deliberate skip is a successful plan with an unset occurrence.
      return true;
    default:
      // Invalid input, unsupported reverse resolution, or an out-of-range
      // candidate is an error for the caller to handle, not a deliberate skip.
      return false;
  }
}

// Plan calendar days, rather than repeatedly adding a 24-hour duration.
void PrintPlan(CivilDay first_day) {
  for (int i = 0; i < 3; ++i) {
    // Advance dates, then resolve 02:30 again for each date. Adding Hours(24)
    // to yesterday's instant can move the local hour across a clock change.
    const CivilDay date = first_day.addDays(i);

    // %F is YYYY-MM-DD in calendar formats; the extra byte is for the NUL.
    char day[11];
    if (FormatCivilDay(date, "%F", day, sizeof(day)).status != TextStatus::kOk)
      return;
    WallTime occurrence = WallTime::Unset();
    if (!PlanDay(date, TimeOfDay::FromHms(2, 30), schedule_zone, occurrence)) {
      Serial.printf("%s: resolution error\n", day);
    } else if (!occurrence.isSet()) {
      Serial.printf("%s: skipped (clock jumps over 02:30)\n", day);
    } else {
      // Show the selected instant in UTC so the overlap choice is visible.
      // Arduino String formatting keeps printing concise in this planning demo.
      Serial.printf("%s 02:30 local -> ", day);
      Serial.println(
          FormatIsoDateTimeArduino(DateTime(occurrence, timezone::UTC)));
    }
  }
}

void setup() {
  Serial.begin(115200);

  // March 29 skips 02:30. October 25 repeats it; our policy picks 00:30 UTC,
  // the earlier of the two matching instants on that date.
  PrintPlan(CivilDay::FromYmd(2026, 3, 28));
  PrintPlan(CivilDay::FromYmd(2026, 10, 24));
}

void loop() {
  // The calendar plans were printed once in setup().
  Delay(Seconds(1));
}
