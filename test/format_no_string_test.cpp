// This test can also compile with a poisoned <string> header and -U__linux__
// (which disables the core's Linux-only ostream test printers).
#define ROO_TIME_HAS_STD_STRING 0
#define ROO_TIME_HAS_STRING_VIEW 0
#include "roo_time/format.h"

int main() {
  using namespace roo_time;
  char buffer[32];
  DateTime output;
  const char input[] = {'2', '0', '2', '4', '-', '0', '2', '-', '2', '9'};
  const char format[] = {'%', 'F'};
  auto parsed = ParseDateTime(input, sizeof(input), format, sizeof(format),
                              timezone::UTC, &output);
  if (parsed.status != TextStatus::kOk || output.day() != 29) return 1;
  auto formatted = FormatDateTime(output, "%F", buffer, sizeof(buffer));
  if (formatted.status != TextStatus::kOk ||
      std::strcmp(buffer, "2024-02-29") != 0)
    return 2;
  char iso[kIsoDateTimeBufferSize];
  if (FormatIsoDateTime(output, iso, sizeof(iso)).status != TextStatus::kOk)
    return 3;
  DateTime restored;
  if (ParseIsoDateTime(iso, &restored).status != TextStatus::kOk ||
      restored != output)
    return 4;
  FixedTimeZone zone(UtcOffset(Hours(1)));
  auto zoned = FormatIsoDateTime(output.wallTime(), zone, iso, sizeof(iso));
  if (zoned.status != TextStatus::kOk ||
      std::strcmp(iso, "2024-02-29T01:00:00.000000+01:00") != 0) return 5;
  return 0;
}
