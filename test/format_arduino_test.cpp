#ifndef ARDUINO
#define ARDUINO 1
#endif
#define ROO_TIME_HAS_STD_STRING 0
#define ROO_TIME_HAS_STRING_VIEW 0
#include "roo_time/format.h"

bool String::fail_allocation = false;

int main() {
  using namespace roo_time;
  const DateTime value(2024, 2, 29, 12, 34, 56, 123456, UtcOffset(Hours(2)));
  if (std::strcmp(FormatDateTimeArduino(value, "%FT%T.%f%:z").c_str(),
                  "2024-02-29T12:34:56.123456+02:00") != 0)
    return 1;
  const char format[] = {'%', 'F'};
  if (std::strcmp(FormatDateTimeArduino(value, format, sizeof(format)).c_str(),
                  "2024-02-29") != 0)
    return 2;
  char long_format[201];
  std::memset(long_format, 'x', 200);
  long_format[200] = '\0';
  if (std::strcmp(FormatDateTimeArduino(value, long_format).c_str(),
                  long_format) != 0)
    return 3;
  if (std::strcmp(FormatDateTimeArduino(value, "%Q").c_str(), "") != 0)
    return 4;
  if (std::strcmp(FormatDateTimeArduino(value, "").c_str(), "") != 0) return 5;
  if (std::strcmp(FormatDateTimeArduino(value, nullptr).c_str(), "") != 0)
    return 6;
  String iso = FormatIsoDateTimeArduino(value);
  if (std::strcmp(iso.c_str(), "2024-02-29T12:34:56.123456+02:00") != 0)
    return 9;
  DateTime parsed;
  if (ParseIsoDateTime(iso, &parsed).status != TextStatus::kOk ||
      parsed != value)
    return 10;
  FixedTimeZone zone(UtcOffset(Minutes(345)));
  if (std::strcmp(FormatIsoDateTimeArduino(value.wallTime(), zone).c_str(),
                  "2024-02-29T16:19:56.123456+05:45") != 0)
    return 12;
  // Verifies duration adapters cover defaults, compact fractions, and long
  // output.
  if (std::strcmp(FormatDurationArduino(Millis(1)).c_str(), "00:00:00.001") !=
      0)
    return 13;
  if (std::strcmp(
          FormatDurationArduino(Micros(1), kDurationCompactFormat).c_str(),
          "0.000001") != 0)
    return 14;
  if (std::strcmp(FormatDurationArduino(Seconds(1), long_format).c_str(),
                  long_format) != 0)
    return 15;
  const char duration_format[] = {'%', 'T', '%', 'F'};
  if (std::strcmp(FormatDurationArduino(Millis(1), duration_format,
                                        sizeof(duration_format))
                      .c_str(),
                  "0.001") != 0)
    return 16;
  if (std::strcmp(FormatDurationArduino(Seconds(1), "%Q").c_str(), "") != 0 ||
      std::strcmp(FormatDurationArduino(Seconds(1), nullptr).c_str(), "") !=
          0 ||
      std::strcmp(FormatDurationArduino(Seconds(-1), "").c_str(), "") != 0)
    return 17;
  String::fail_allocation = true;
  if (std::strcmp(FormatDateTimeArduino(value, "%F").c_str(), "") != 0)
    return 7;
  if (std::strcmp(FormatDateTimeArduino(value, long_format).c_str(), "") != 0)
    return 8;
  if (std::strcmp(FormatIsoDateTimeArduino(value).c_str(), "") != 0) return 11;
  // Verifies String allocation failures produce empty duration output.
  if (std::strcmp(FormatDurationArduino(Millis(1)).c_str(), "") != 0 ||
      std::strcmp(FormatDurationArduino(Seconds(1), long_format).c_str(), "") !=
          0)
    return 18;
  return 0;
}
