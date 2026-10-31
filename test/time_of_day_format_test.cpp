#include <cstring>

#include "gtest/gtest.h"
#include "roo_time/format.h"

namespace roo_time {
namespace {

// Verifies all clock fields and fractional boundaries round-trip exactly.
TEST(TimeOfDayText, RoundTripAndNumericSuffixes) {
  for (int hour = 0; hour < 24; ++hour) {
    for (int minute : {0, 30, 59}) {
      for (int second : {0, 59}) {
        for (int micros : {0, 1, 32768, 65536, 500000, 999999}) {
          const TimeOfDay original =
              TimeOfDay::FromHms(hour, minute, second, micros);
          for (const char* format : {"%T.%f", "%H%M%S%f", "%T.%f%S0"}) {
            char text[32];
            const FormatResult formatted =
                FormatTimeOfDay(original, format, text, sizeof(text));
            ASSERT_EQ(TextStatus::kOk, formatted.status);
            TimeOfDay parsed;
            const ParseResult result =
                ParseTimeOfDay(text, formatted.size, format, &parsed);
            ASSERT_EQ(TextStatus::kOk, result.status) << text;
            EXPECT_EQ(formatted.size, result.position);
            EXPECT_EQ(original, parsed);
          }
        }
      }
    }
  }
}

// Verifies short clock forms, repeated fields, and decimal fraction scaling.
TEST(TimeOfDayText, DefaultsFractionsAndRepeatedFields) {
  TimeOfDay parsed;
  ASSERT_EQ(TextStatus::kOk,
            ParseTimeOfDay("06:30", 5, "%H:%M", &parsed).status);
  EXPECT_EQ(TimeOfDay::FromHms(6, 30), parsed);
  ASSERT_EQ(TextStatus::kOk,
            ParseTimeOfDay("06:30 06 %", 10, "%H:%M %H %%", &parsed).status);
  EXPECT_EQ(TimeOfDay::FromHms(6, 30), parsed);
  const char* fractions[] = {"12:34:56.1",     "12:34:56.12",
                             "12:34:56.123",   "12:34:56.1234",
                             "12:34:56.12345", "12:34:56.123456"};
  const uint32_t expected[] = {100000, 120000, 123000, 123400, 123450, 123456};
  for (int i = 0; i < 6; ++i) {
    ASSERT_EQ(TextStatus::kOk,
              ParseTimeOfDay(fractions[i], std::strlen(fractions[i]), "%T.%f",
                             &parsed)
                  .status);
    EXPECT_EQ(TimeOfDay::FromHms(12, 34, 56, expected[i]), parsed);
  }
}

// Verifies size queries, every short buffer, NUL termination, and no
// overwrites.
TEST(TimeOfDayText, BoundedOutput) {
  const TimeOfDay value = TimeOfDay::FromHms(23, 59, 59, 999999);
  const char* expected = "23:59:59.999999";
  const FormatResult measured = FormatTimeOfDay(value, "%T.%f", nullptr, 0);
  EXPECT_EQ(TextStatus::kBufferTooSmall, measured.status);
  EXPECT_EQ(15u, measured.size);
  for (size_t capacity = 0; capacity <= 17; ++capacity) {
    char buffer[20];
    std::memset(buffer, 'x', sizeof(buffer));
    const FormatResult result =
        FormatTimeOfDay(value, "%T.%f", buffer, capacity);
    EXPECT_EQ(15u, result.size);
    EXPECT_EQ(capacity > 15 ? TextStatus::kOk : TextStatus::kBufferTooSmall,
              result.status);
    EXPECT_EQ('x', buffer[capacity]);
    if (capacity != 0) {
      const size_t written = capacity > 15 ? 15 : capacity - 1;
      EXPECT_EQ(0, std::strncmp(expected, buffer, written));
      EXPECT_EQ('\0', buffer[written]);
    }
  }
  EXPECT_EQ(TextStatus::kInvalidInput,
            FormatTimeOfDay(value, "%T", nullptr, 1).status);
  char output = 'x';
  EXPECT_EQ(TextStatus::kOk, FormatTimeOfDay(value, "", &output, 1).status);
  EXPECT_EQ('\0', output);
}

// Verifies out-of-range, malformed, conflicting, and missing fields preserve
// output.
TEST(TimeOfDayText, InvalidInputPreservesDestination) {
  const TimeOfDay original = TimeOfDay::FromHms(6, 30);
  struct Case {
    const char* text;
    const char* format;
    TextStatus status;
  };
  const Case cases[] = {
      {"24:00", "%H:%M", TextStatus::kOutOfRange},
      {"23:60", "%H:%M", TextStatus::kOutOfRange},
      {"23:59:60", "%T", TextStatus::kOutOfRange},
      {"-1:00", "%H:%M", TextStatus::kInvalidInput},
      {"6:30", "%H:%M", TextStatus::kInvalidInput},
      {"06:30 ", "%H:%M", TextStatus::kInvalidInput},
      {"06:30:00.", "%T.%f", TextStatus::kInvalidInput},
      {"06:30:00.1234567", "%T.%f", TextStatus::kInvalidInput},
      {"06:30/07", "%H:%M/%H", TextStatus::kInvalidInput},
      {"06:30.1/.2", "%H:%M.%f/.%f", TextStatus::kInvalidInput},
      {"06", "%H", TextStatus::kInvalidInput},
      {"30", "%M", TextStatus::kInvalidInput},
      {"", "", TextStatus::kInvalidInput},
  };
  for (const Case& c : cases) {
    TimeOfDay result = original;
    const ParseResult parsed =
        ParseTimeOfDay(c.text, std::strlen(c.text), c.format, &result);
    EXPECT_EQ(c.status, parsed.status) << c.text;
    EXPECT_LE(parsed.position, std::strlen(c.text));
    EXPECT_EQ(original, result);
  }
  TimeOfDay result = original;
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseTimeOfDay(nullptr, 5, "%H:%M", &result).status);
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseTimeOfDay(nullptr, 0, "%H:%M", &result).status);
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseTimeOfDay("06:30", 5, "%H:%M", nullptr).status);
  EXPECT_EQ(original, result);
}

// Verifies date/offset directives and malformed formats are rejected
// consistently.
TEST(TimeOfDayText, InvalidFormatsAndValues) {
  const TimeOfDay original = TimeOfDay::FromHms(6, 30);
  const char* invalid_formats[] = {"%Y",  "%m", "%d", "%F",  "%z",
                                   "%:z", "%Q", "%",  "%:x", nullptr};
  for (const char* format : invalid_formats) {
    char text[32] = "unchanged";
    EXPECT_EQ(TextStatus::kInvalidFormat,
              FormatTimeOfDay(original, format, text, sizeof(text)).status);
    EXPECT_EQ('\0', text[0]);
    TimeOfDay parsed = original;
    EXPECT_EQ(TextStatus::kInvalidFormat,
              ParseTimeOfDay("06:30", 5, format, &parsed).status);
    EXPECT_EQ(original, parsed);
  }
  char text[8] = "old";
  EXPECT_EQ(
      TextStatus::kInvalidInput,
      FormatTimeOfDay(TimeOfDay::Invalid(), "%T", text, sizeof(text)).status);
  EXPECT_EQ('\0', text[0]);
}

// Verifies bounded ranges and views neither need nor consume NUL terminators.
TEST(TimeOfDayText, BoundedInputAndFormat) {
  const char input[] = {'0', '6', ':', '3', '0', 'x'};
  const char format[] = {'%', 'H', ':', '%', 'M', 'x'};
  TimeOfDay parsed;
  ASSERT_EQ(TextStatus::kOk,
            ParseTimeOfDay(input, 5, format, 5, &parsed).status);
  EXPECT_EQ(TimeOfDay::FromHms(6, 30), parsed);
  char text[6];
  EXPECT_EQ(TextStatus::kOk,
            FormatTimeOfDay(parsed, format, 5, text, sizeof(text)).status);
  EXPECT_STREQ("06:30", text);
#if ROO_TIME_HAS_STRING_VIEW
  EXPECT_EQ(TextStatus::kOk,
            ParseTimeOfDay(roo::string_view(input, 5),
                           roo::string_view(format, 5), &parsed)
                .status);
  EXPECT_EQ(
      TextStatus::kOk,
      FormatTimeOfDay(parsed, roo::string_view(format, 5), text, sizeof(text))
          .status);
#endif
  const char embedded_null[] = {'%', 'H', '\0', '%', 'M'};
  EXPECT_EQ(TextStatus::kInvalidFormat,
            FormatTimeOfDay(parsed, embedded_null, sizeof(embedded_null), text,
                            sizeof(text))
                .status);
  const TimeOfDay original = parsed;
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseTimeOfDay(input, 4, "%H:%M", &parsed).status);
  EXPECT_EQ(original, parsed);
}

}  // namespace
}  // namespace roo_time
