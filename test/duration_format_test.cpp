#include <cstring>

#include "gtest/gtest.h"
#include "roo_time/format.h"

namespace roo_time {
namespace {

// Verifies successful output and the reported byte count together.
void ExpectFormatted(Duration value, const char* format, const char* expected) {
  char text[384];
  const FormatResult result = FormatDuration(value, format, text, sizeof(text));
  ASSERT_EQ(TextStatus::kOk, result.status);
  EXPECT_STREQ(expected, text);
  EXPECT_EQ(std::strlen(expected), result.size);
}

// Verifies each named layout, default format, and automatic negative signs.
TEST(DurationText, NamedFormats) {
  const Duration value = Hours(76) + Minutes(5) + Seconds(6) + Millis(1);
  ExpectFormatted(value, kDurationClockFormat, "76:05:06.001");
  ExpectFormatted(value, kDurationCompactFormat, "76:05:06.001");
  ExpectFormatted(value, kDurationDaysClockFormat, "3d 04:05:06.001");
  ExpectFormatted(value, kDurationClockMillisFormat, "76:05:06.001");
  ExpectFormatted(value, kDurationClockMicrosFormat, "76:05:06.001000");
  ExpectFormatted(value * -1, kDurationDaysClockFormat, "-3d 04:05:06.001");
  ExpectFormatted(Micros(0), kDurationClockFormat, "00:00:00");
  ExpectFormatted(Seconds(5), kDurationDaysClockFormat, "0d 00:00:05");
  char text[32];
  ASSERT_EQ(TextStatus::kOk, FormatDuration(value, text, sizeof(text)).status);
  EXPECT_STREQ("76:05:06.001", text);
  ExpectFormatted(SmallMillis(1), kDurationClockFormat, "00:00:00.001");
}

// Verifies compact clocks omit only leading components and retain trailing
// zeros.
TEST(DurationText, CompactClock) {
  struct Case {
    Duration value;
    const char* text;
  };
  const Case cases[] = {
      {Micros(0), "0"},        {Seconds(25), "25"},
      {Seconds(85), "1:25"},   {Hours(1) + Minutes(5), "1:05:00"},
      {Hours(27), "27:00:00"}, {Micros(1), "0.000001"},
      {Millis(1), "0.001"},    {Seconds(85) + Millis(1), "1:25.001"}};
  for (const Case& c : cases) {
    ExpectFormatted(c.value, kDurationCompactFormat, c.text);
    if (c.value.inMicros() != 0) {
      char expected[64] = "-";
      std::strcat(expected, c.text);
      ExpectFormatted(c.value * -1, kDurationCompactFormat, expected);
    }
  }
}

// Verifies trimmed fractions preserve leading zeros and fixed fractions
// truncate.
TEST(DurationText, FractionsAndPrecision) {
  struct Case {
    int32_t micros;
    const char* fraction;
  };
  const Case cases[] = {{0, ""},        {1, ".000001"},   {10, ".00001"},
                        {100, ".0001"}, {1000, ".001"},   {10000, ".01"},
                        {100000, ".1"}, {1010, ".00101"}, {999999, ".999999"}};
  for (const Case& c : cases)
    ExpectFormatted(Micros(c.micros), "%F", c.fraction);
  ExpectFormatted(Micros(0), "%f", "000000");
  ExpectFormatted(Micros(1), "%f", "000001");
  ExpectFormatted(Micros(123456), "%.1f/%.2f/%.3f/%.4f/%.5f/%.6f",
                  "1/12/123/1234/12345/123456");
  ExpectFormatted(Micros(1999999), "%T.%.3f", "1.999");
  ExpectFormatted(Micros(-1), kDurationClockMillisFormat, "-00:00:00.000");
  ExpectFormatted(Micros(-1), "%F", "-.000001");
  ExpectFormatted(Seconds(-1), "%F", "");
}

// Verifies explicit signs suppress automatic signs even when placed after
// fields.
TEST(DurationText, ExplicitSignPlacement) {
  ExpectFormatted(Micros(0), "%+", "+");
  ExpectFormatted(Seconds(1), "S%+%T", "S+1");
  ExpectFormatted(Seconds(-1), "S%+%T", "S-1");
  ExpectFormatted(Seconds(-1), "S%T", "-S1");
  ExpectFormatted(Seconds(-1), "%T%+", "1-");
  ExpectFormatted(Seconds(-1), "%+%T/%+", "-1/-");
  ExpectFormatted(Seconds(-1), "%%+%T", "-%+1");
  ExpectFormatted(Seconds(-1), "%+H", "-H");
  ExpectFormatted(Micros(-1), "S%+%T.%.3f", "S-0.000");
  ExpectFormatted(Duration::Min(), kUptimeClockMicrosFormat,
                  "S-2562047788:00:54.775808");
  ExpectFormatted(Duration::Max(), kUptimeClockMicrosFormat,
                  "S+2562047788:00:54.775807");
  const char format[] = {'%', 'T', '%', '+'};
  char text[32];
  ASSERT_EQ(TextStatus::kOk,
            FormatDuration(Seconds(-1), format, 2, text, sizeof(text)).status);
  EXPECT_STREQ("-1", text);
  ASSERT_EQ(TextStatus::kOk,
            FormatDuration(Seconds(-1), format, 4, text, sizeof(text)).status);
  EXPECT_STREQ("1-", text);
}

// Verifies integer padding modifiers, minimum widths, and nontruncating fields.
TEST(DurationText, WidthsAndUnpaddedComponents) {
  const Duration value = Hours(76) + Minutes(5) + Seconds(6);
  ExpectFormatted(value, "%D/%H/%h/%M/%S", "3/76/04/05/06");
  ExpectFormatted(value, "%-D/%-H/%-h/%-M/%-S", "3/76/4/5/6");
  ExpectFormatted(value, "%03D/%03h/%03M/%03S", "003/004/005/006");
  ExpectFormatted(value, "%3D/%3h/%3M/%3S", "  3/  4/  5/  6");
  ExpectFormatted(Hours(1), "%-H:%M:%S", "1:00:00");
  ExpectFormatted(Hours(24 * 123), "%02D/%01H", "123/2952");
  ExpectFormatted(value * -1, "%03D days %%", "-003 days %");
  char text[256];
  const FormatResult result =
      FormatDuration(Hours(24), "%0255D", text, sizeof(text));
  ASSERT_EQ(TextStatus::kOk, result.status);
  EXPECT_EQ(255u, result.size);
  for (size_t i = 0; i < 254; ++i) EXPECT_EQ('0', text[i]);
  EXPECT_EQ('1', text[254]);
  EXPECT_EQ('\0', text[255]);
}

// Verifies formatting avoids signed negation overflow and component saturation.
TEST(DurationText, FullDurationRange) {
  ExpectFormatted(Duration::Max(), kDurationClockFormat,
                  "2562047788:00:54.775807");
  ExpectFormatted(Duration::Min(), kDurationClockFormat,
                  "-2562047788:00:54.775808");
  ExpectFormatted(Duration::Max(), kDurationDaysClockFormat,
                  "106751991d 04:00:54.775807");
  ExpectFormatted(Duration::Min(), kDurationDaysClockFormat,
                  "-106751991d 04:00:54.775808");
  ExpectFormatted(Duration::Min(), kDurationCompactFormat,
                  "-2562047788:00:54.775808");
}

// Verifies every short buffer, size queries, and no writes beyond capacity.
TEST(DurationText, BoundedBuffers) {
  const Duration value = Micros(-1);
  const char* expected = "-00:00:00.000001";
  const size_t length = std::strlen(expected);
  EXPECT_EQ(TextStatus::kBufferTooSmall,
            FormatDuration(value, nullptr, 0).status);
  EXPECT_EQ(length, FormatDuration(value, nullptr, 0).size);
  for (size_t capacity = 0; capacity <= length + 2; ++capacity) {
    char text[32];
    std::memset(text, 'x', sizeof(text));
    const FormatResult result = FormatDuration(value, text, capacity);
    EXPECT_EQ(capacity > length ? TextStatus::kOk : TextStatus::kBufferTooSmall,
              result.status);
    EXPECT_EQ(length, result.size);
    EXPECT_EQ('x', text[capacity]);
    if (capacity != 0) {
      const size_t written = capacity > length ? length : capacity - 1;
      EXPECT_EQ(0, std::strncmp(expected, text, written));
      EXPECT_EQ('\0', text[written]);
    }
  }
  EXPECT_EQ(TextStatus::kInvalidInput,
            FormatDuration(value, nullptr, 1).status);
  ExpectFormatted(Seconds(-1), "", "");
  ExpectFormatted(Seconds(-1), "literal", "-literal");
}

// Verifies malformed directives are rejected before emitting partial output.
TEST(DurationText, InvalidFormats) {
  const char* formats[] = {nullptr,
                           "%",
                           "%Y",
                           "%:z",
                           "%Q",
                           "%0D",
                           "%00H",
                           "%256D",
                           "%999999999999999999999H",
                           "%-03D",
                           "%--H",
                           "%03T",
                           "%-f",
                           "%2F",
                           "%.0f",
                           "%.7f",
                           "%.3H",
                           "%.12f",
                           "%.f",
                           "%03+",
                           "%-+",
                           "%.3+",
                           "prefix %H bad %"};
  for (const char* format : formats) {
    char text[32] = "unchanged";
    const FormatResult result =
        FormatDuration(Seconds(-1), format, text, sizeof(text));
    EXPECT_EQ(TextStatus::kInvalidFormat, result.status);
    EXPECT_EQ(0u, result.size);
    EXPECT_EQ('\0', text[0]);
  }
  const char embedded_null[] = {'%', 'H', '\0', '%', 'M'};
  char text[16] = "old";
  EXPECT_EQ(TextStatus::kInvalidFormat,
            FormatDuration(Seconds(1), embedded_null, sizeof(embedded_null),
                           text, sizeof(text))
                .status);
  EXPECT_EQ('\0', text[0]);
}

// Verifies bounded format views and null empty ranges obey existing contracts.
TEST(DurationText, BoundedFormats) {
  const char format[] = {'%', 'T', '%', 'F', 'x'};
  char text[16];
  ASSERT_EQ(TextStatus::kOk,
            FormatDuration(Millis(1), format, 4, text, sizeof(text)).status);
  EXPECT_STREQ("0.001", text);
#if ROO_TIME_HAS_STRING_VIEW
  ASSERT_EQ(
      TextStatus::kOk,
      FormatDuration(Millis(1), roo::string_view(format, 4), text, sizeof(text))
          .status);
  EXPECT_STREQ("0.001", text);
#endif
  ASSERT_EQ(TextStatus::kOk,
            FormatDuration(Seconds(-1), nullptr, 0, text, sizeof(text)).status);
  EXPECT_STREQ("", text);
}

#if ROO_TIME_HAS_STD_STRING
// Verifies allocating overloads share buffer semantics and keep default queries
// clear.
TEST(DurationText, StringConveniences) {
  EXPECT_EQ("00:00:00.001", FormatDuration(Millis(1)));
  EXPECT_EQ("1:25.001",
            FormatDuration(Seconds(85) + Millis(1), kDurationCompactFormat));
  EXPECT_EQ("-2562047788:00:54.775808", FormatDuration(Duration::Min()));
  EXPECT_EQ("", FormatDuration(Seconds(1), "%Q"));
  EXPECT_EQ("", FormatDuration(Seconds(-1), ""));
  EXPECT_EQ("", FormatDuration(Seconds(1), static_cast<const char*>(nullptr)));
  EXPECT_EQ(255u, FormatDuration(Hours(24), "%0255D").size());
  EXPECT_EQ(TextStatus::kBufferTooSmall,
            FormatDuration(Millis(1), nullptr, 0).status);
#if ROO_TIME_HAS_STRING_VIEW
  const char format[] = {'%', 'T', '%', 'F', 'x'};
  EXPECT_EQ("0.001", FormatDuration(Millis(1), roo::string_view(format, 4)));
  EXPECT_EQ("", FormatDuration(Seconds(-1), roo::string_view()));
#endif
}
#endif

}  // namespace
}  // namespace roo_time
