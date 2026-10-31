#include <cstring>

#include "gtest/gtest.h"
#include "roo_time/format.h"

namespace roo_time {
namespace {

// Verifies the emitted bytes and required size together for an uptime layout.
void ExpectUptime(Uptime value, const char* format, const char* expected) {
  char text[384];
  const FormatResult result = FormatUptime(value, format, text, sizeof(text));
  ASSERT_EQ(TextStatus::kOk, result.status);
  EXPECT_STREQ(expected, text);
  EXPECT_EQ(std::strlen(expected), result.size);
}

// Verifies the default retains the domain marker, uses total hours, and is
// exact.
TEST(UptimeText, DefaultAndFullRange) {
  EXPECT_STREQ("S%+%H:%M:%S.%f", kUptimeClockMicrosFormat);
  char text[48];
  ASSERT_EQ(TextStatus::kOk,
            FormatUptime(Uptime::Start(), text, sizeof(text)).status);
  EXPECT_STREQ("S+00:00:00.000000", text);
  const Uptime stamp =
      Uptime::Start() + Hours(27) + Minutes(5) + Seconds(6) + Millis(1);
  ASSERT_EQ(TextStatus::kOk, FormatUptime(stamp, text, sizeof(text)).status);
  EXPECT_STREQ("S+27:05:06.001000", text);
  ExpectUptime(Uptime::Start() - Micros(1), kUptimeClockMicrosFormat,
               "S-00:00:00.000001");
  ExpectUptime(Uptime::Max(), kUptimeClockMicrosFormat,
               "S+2562047788:00:54.775807");
  ExpectUptime(Uptime::Start() + Duration::Min(), kUptimeClockMicrosFormat,
               "S-2562047788:00:54.775808");
}

// Verifies custom layouts control the entire output, including marker and sign.
TEST(UptimeText, ReusesNumericDurationFormats) {
  ExpectUptime(Uptime::Start() + Seconds(85) + Millis(1),
               kDurationCompactFormat, "1:25.001");
  ExpectUptime(Uptime::Start() - Seconds(85) - Millis(1),
               kDurationCompactFormat, "-1:25.001");
  ExpectUptime(Uptime::Start() + Hours(76), kDurationDaysClockFormat,
               "3d 04:00:00");
  ExpectUptime(Uptime::Start() + Hours(76) + Minutes(5), "%03D %-h:%M:%S",
               "003 4:05:00");
  ExpectUptime(Uptime::Start() - Micros(1), kDurationClockMillisFormat,
               "-00:00:00.000");
  ExpectUptime(Uptime::Start() - Seconds(1), "label %T %%", "-label 1 %");
  ExpectUptime(Uptime::Start() - Seconds(85) - Millis(1), "S%+%T%F",
               "S-1:25.001");
  ExpectUptime(Uptime::Start() + Seconds(85), "S%+%T", "S+1:25");
  ExpectUptime(Uptime::Start() - Seconds(1), "S%T", "-S1");
  ExpectUptime(Uptime::Start() - Seconds(1), "%T%+", "1-");
}

// Verifies truncation inside the marker, sign, and timestamp keeps buffer
// bounds.
TEST(UptimeText, BoundedBuffersAndSizeQueries) {
  const Uptime value = Uptime::Start() - Micros(1);
  const char* expected = "S-00:00:00.000001";
  const size_t size = std::strlen(expected);
  const FormatResult measured = FormatUptime(value, nullptr, 0);
  EXPECT_EQ(TextStatus::kBufferTooSmall, measured.status);
  EXPECT_EQ(size, measured.size);
  for (size_t capacity = 0; capacity <= size + 2; ++capacity) {
    char text[32];
    std::memset(text, 'x', sizeof(text));
    const FormatResult result = FormatUptime(value, text, capacity);
    EXPECT_EQ(size, result.size);
    EXPECT_EQ(capacity > size ? TextStatus::kOk : TextStatus::kBufferTooSmall,
              result.status);
    EXPECT_EQ('x', text[capacity]);
    if (capacity != 0) {
      const size_t written = capacity > size ? size : capacity - 1;
      EXPECT_EQ(0, std::strncmp(expected, text, written));
      EXPECT_EQ('\0', text[written]);
    }
  }
  EXPECT_EQ(TextStatus::kInvalidInput, FormatUptime(value, nullptr, 1).status);
}

// Verifies invalid layouts never leave a marker or partial numeric output.
TEST(UptimeText, InvalidFormats) {
  const char* invalid[] = {nullptr, "%", "%Q", "%256H", "%03T", "%.7f", "ok%Y"};
  for (const char* format : invalid) {
    char text[32] = "old";
    const FormatResult result =
        FormatUptime(Uptime::Start(), format, text, sizeof(text));
    EXPECT_EQ(TextStatus::kInvalidFormat, result.status);
    EXPECT_EQ(0u, result.size);
    EXPECT_EQ('\0', text[0]);
  }
  const char embedded_null[] = {'%', 'T', '\0'};
  char text[32] = "old";
  EXPECT_EQ(TextStatus::kInvalidFormat,
            FormatUptime(Uptime::Start(), embedded_null, sizeof(embedded_null),
                         text, sizeof(text))
                .status);
  EXPECT_EQ('\0', text[0]);
}

// Verifies empty complete layouts stay empty and bounded views control output.
TEST(UptimeText, EmptyAndBoundedLayouts) {
  ExpectUptime(Uptime::Start(), "", "");
  ExpectUptime(Uptime::Start() - Seconds(1), "", "");
  ExpectUptime(Uptime::Start(), "%F", "");
  char text[32];
  ASSERT_EQ(
      TextStatus::kOk,
      FormatUptime(Uptime::Start(), nullptr, 0, text, sizeof(text)).status);
  EXPECT_STREQ("", text);
  const char format[] = {'%', 'T', '%', 'F', 'x'};
  const Uptime value = Uptime::Start() + Millis(1);
  ASSERT_EQ(TextStatus::kOk,
            FormatUptime(value, format, 4, text, sizeof(text)).status);
  EXPECT_STREQ("0.001", text);
#if ROO_TIME_HAS_STRING_VIEW
  ASSERT_EQ(TextStatus::kOk,
            FormatUptime(value, roo::string_view(format, 4), text, sizeof(text))
                .status);
  EXPECT_STREQ("0.001", text);
#endif
}

#if ROO_TIME_HAS_STD_STRING
// Verifies owning overloads follow defaults and custom complete layouts.
TEST(UptimeText, StringConveniences) {
  EXPECT_EQ("S+00:00:00.000000", FormatUptime(Uptime::Start()));
  EXPECT_EQ("-0.001",
            FormatUptime(Uptime::Start() - Millis(1), kDurationCompactFormat));
  EXPECT_EQ("S-2562047788:00:54.775808",
            FormatUptime(Uptime::Start() + Duration::Min()));
  EXPECT_EQ(255u, FormatUptime(Uptime::Start(), "%0255H").size());
  EXPECT_EQ("", FormatUptime(Uptime::Start(), ""));
  EXPECT_EQ("", FormatUptime(Uptime::Start(), "%Q"));
  EXPECT_EQ("",
            FormatUptime(Uptime::Start(), static_cast<const char*>(nullptr)));
#if ROO_TIME_HAS_STRING_VIEW
  const char format[] = {'%', 'T', 'x'};
  EXPECT_EQ("0", FormatUptime(Uptime::Start(), roo::string_view(format, 2)));
#endif
}
#endif

}  // namespace
}  // namespace roo_time
