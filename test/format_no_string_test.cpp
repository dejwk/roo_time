// This test can also compile with a poisoned <string> header and -U__linux__
// (which disables the core's Linux-only ostream test printers).
#define ROO_TIME_HAS_STD_STRING 0
#define ROO_TIME_HAS_STRING_VIEW 0
#include "roo_time/format.h"

namespace {

// Verifies fractions retain their full range, including on 16-bit-int targets.
bool CheckFractions() {
  using namespace roo_time;
  struct Case {
    uint32_t micros;
    const char* text;
  };
  const Case cases[] = {{32767, "032767"},  {32768, "032768"},
                        {65535, "065535"},  {65536, "065536"},
                        {500000, "500000"}, {999999, "999999"}};
  for (const Case& c : cases) {
    const DateTime value(2024, 2, 29, 0, 0, 0, c.micros, timezone::UTC);
    char buffer[7];
    if (FormatDateTime(value, "%f", buffer, sizeof(buffer)).status !=
            TextStatus::kOk ||
        std::strcmp(buffer, c.text) != 0)
      return false;

    char input[] = "2024-02-29.000000";
    std::memcpy(input + 11, c.text, 6);
    DateTime parsed;
    if (ParseDateTime(input, sizeof(input) - 1, "%F.%f", timezone::UTC, &parsed)
                .status != TextStatus::kOk ||
        parsed != value)
      return false;
  }

  DateTime parsed;
  const char scaled[] = "2024-02-29.5";
  if (ParseDateTime(scaled, sizeof(scaled) - 1, "%F.%f", timezone::UTC, &parsed)
              .status != TextStatus::kOk ||
      parsed.micros() != 500000UL)
    return false;

  // These fractions have identical low 16 bits but must not compare equal.
  const DateTime original = parsed;
  const char conflicting[] = "2024-02-29.016959/999999";
  return ParseDateTime(conflicting, sizeof(conflicting) - 1, "%F.%f/%f",
                       timezone::UTC, &parsed)
                 .status == TextStatus::kInvalidInput &&
         parsed == original;
}

}  // namespace

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
      std::strcmp(iso, "2024-02-29T01:00:00.000000+01:00") != 0)
    return 5;
  CivilDay date;
  if (ParseCivilDay(input, sizeof(input), format, sizeof(format), &date)
              .status != TextStatus::kOk ||
      date != output.civilDay())
    return 7;
  if (FormatCivilDay(date, format, sizeof(format), buffer, sizeof(buffer))
              .status != TextStatus::kOk ||
      std::strcmp(buffer, "2024-02-29") != 0)
    return 8;
  return CheckFractions() ? 0 : 6;
}
