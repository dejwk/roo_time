#include "roo_time/format.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

namespace roo_time {
namespace {

const char kFormat[] = "%FT%T.%f%:z";

TEST(IsoDateTime, CanonicalOutputAndRoundTrip) {
  for (int offset : {-1439, -330, 0, 345, 1439}) {
    for (int micros : {0, 1, 123456, 999999}) {
      DateTime value(2024, 2, 29, 12, 34, 56, micros,
                     UtcOffset(Minutes(offset)));
      char buffer[kIsoDateTimeBufferSize];
      auto formatted = FormatIsoDateTime(value, buffer, sizeof(buffer));
      ASSERT_EQ(TextStatus::kOk, formatted.status);
      EXPECT_EQ(32u, formatted.size);
      EXPECT_EQ(FormatDateTime(value, kFormat), buffer);
      EXPECT_EQ(FormatIsoDateTime(value), buffer);
      DateTime parsed;
      ASSERT_EQ(TextStatus::kOk, ParseIsoDateTime(buffer, &parsed).status);
      EXPECT_EQ(value, parsed);
    }
  }
  EXPECT_EQ("1970-01-01T00:00:00.000000+00:00", FormatIsoDateTime(DateTime()));
  EXPECT_EQ("0001-01-01T00:00:00.000000+00:00",
            FormatIsoDateTime(DateTime(1, 1, 1, timezone::UTC)));
  char small[5];
  auto truncated = FormatIsoDateTime(DateTime(), small, sizeof(small));
  EXPECT_EQ(TextStatus::kBufferTooSmall, truncated.status);
  EXPECT_EQ(32u, truncated.size);
  EXPECT_STREQ("1970", small);
  EXPECT_EQ(32u, FormatIsoDateTime(DateTime(), nullptr, 0).size);
  EXPECT_EQ("", FormatIsoDateTime(DateTime(2024, 1, 1, UtcOffset(Hours(24)))));
}

TEST(IsoDateTime, AcceptedFormsAndBoundedViews) {
  DateTime parsed;
  for (const char* text :
       {"2024-02-29T12:34:56Z", "2024-02-29T12:34:56+00:00",
        "2024-02-29T12:34:56.0Z", "2024-02-29T12:34:56.000000+00:00"}) {
    auto result = ParseIsoDateTime(text, &parsed);
    ASSERT_EQ(TextStatus::kOk, result.status) << text;
    EXPECT_EQ(std::strlen(text), result.position);
    EXPECT_EQ(DateTime(2024, 2, 29, 12, 34, 56, 0, timezone::UTC), parsed);
  }
  const char bounded[] = {'2', '0', '2', '4', '-', '0', '2', '-', '2', '9',
                          'T', '1', '2', ':', '3', '4', ':', '5', '6', 'Z'};
  ASSERT_EQ(
      TextStatus::kOk,
      ParseIsoDateTime(roo::string_view(bounded, sizeof(bounded)), &parsed)
          .status);
  for (unsigned digits = 1; digits <= 6; ++digits) {
    std::string text =
        "2024-02-29T12:34:56." + std::string(digits, '1') + "-05:30";
    ASSERT_EQ(TextStatus::kOk, ParseIsoDateTime(text, &parsed).status);
    EXPECT_EQ(-330, parsed.timeZone().offset().inMinutes());
    EXPECT_EQ("2024-02-29T12:34:56." + std::string(digits, '1') +
                  std::string(6 - digits, '0') + "-05:30",
              FormatIsoDateTime(parsed));
  }
}

TEST(IsoDateTime, RejectedFormsPreserveDestination) {
  const DateTime original(2000, 1, 1, timezone::UTC);
  DateTime parsed = original;
  for (const char* text :
       {"", "2024-02-29", "2024-02-29T12:34:56", "2024-02-29 12:34:56Z",
        "2024-02-29t12:34:56Z", "2024-02-29T12:34:56z",
        "2024-02-29T12:34:56+0000", "2024-02-29T12:34:56.Z",
        "2024-02-29T12:34:56.1234567Z", "2024-02-29T12:34:56,123Z",
        "2024-02-29T12:34:56Z ", "2023-02-29T12:34:56Z", "2024-02-29T12:34:60Z",
        "2024-02-29T12:34:56+24:00"}) {
    EXPECT_NE(TextStatus::kOk, ParseIsoDateTime(text, &parsed).status) << text;
    EXPECT_EQ(original, parsed);
  }
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseIsoDateTime(nullptr, &parsed).status);
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseIsoDateTime(nullptr, 0, &parsed).status);
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseIsoDateTime(nullptr, 20, &parsed).status);
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseIsoDateTime(roo::string_view(), &parsed).status);
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseIsoDateTime("2024-02-29T12:34:56Z", nullptr).status);
  EXPECT_EQ(original, parsed);
}

#if defined(ARDUINO)
TEST(IsoDateTime, ArduinoString) {
  DateTime value(2024, 2, 29, 12, 34, 56, 123456, UtcOffset(Hours(2)));
  String text = FormatIsoDateTimeArduino(value);
  EXPECT_STREQ("2024-02-29T12:34:56.123456+02:00", text.c_str());
  DateTime parsed;
  ASSERT_EQ(TextStatus::kOk, ParseIsoDateTime(text, &parsed).status);
  EXPECT_EQ(value, parsed);
}

TEST(FormatDateTime, ArduinoString) {
  DateTime value(2026, 9, 12, 14, 30, 5, 42, UtcOffset(Hours(2)));
  EXPECT_STREQ("2026-09-12T14:30:05.000042+02:00",
               FormatDateTimeArduino(value, kFormat).c_str());
  const char format[] = {'%', 'F'};
  EXPECT_STREQ(
      "2026-09-12",
      FormatDateTimeArduino(value, roo::string_view(format, 2)).c_str());
  std::string long_format(200, 'x');
  EXPECT_STREQ(long_format.c_str(),
               FormatDateTimeArduino(value, long_format.c_str()).c_str());
  EXPECT_STREQ("", FormatDateTimeArduino(value, "%Q").c_str());
  EXPECT_STREQ("", FormatDateTimeArduino(value, "").c_str());
}
#endif

TEST(FormatDateTime, DirectivesAndAliases) {
  DateTime value(2026, 9, 12, 14, 30, 5, 42, UtcOffset(Hours(2)));
  EXPECT_EQ("2026-09-12T14:30:05.000042+02:00", FormatDateTime(value, kFormat));
  EXPECT_EQ("20260912143005000042+0200%",
            FormatDateTime(value, "%Y%m%d%H%M%S%f%z%%"));
  EXPECT_EQ("+02:00/+02:00/+0200", FormatDateTime(value, "%:z/%:z/%z"));
  EXPECT_EQ("2026-09-12/14:30:05/2026-09-12",
            FormatDateTime(value, "%F/%T/%F"));
}

TEST(FormatDateTime, BufferSizesAndSizeQuery) {
  DateTime value;
  auto measured = FormatDateTime(value, "%F", nullptr, 0);
  EXPECT_EQ(TextStatus::kBufferTooSmall, measured.status);
  EXPECT_EQ(10u, measured.size);
  for (size_t capacity = 0; capacity <= 12; ++capacity) {
    char buffer[14];
    std::memset(buffer, '!', sizeof(buffer));
    auto result = FormatDateTime(value, "%F", buffer, capacity);
    EXPECT_EQ(10u, result.size);
    EXPECT_EQ(capacity > 10 ? TextStatus::kOk : TextStatus::kBufferTooSmall,
              result.status);
    if (capacity != 0) {
      EXPECT_EQ(std::string("1970-01-01").substr(0, capacity - 1), buffer);
    }
    EXPECT_EQ('!', buffer[capacity]);
  }
  char buffer[1] = {'!'};
  EXPECT_EQ(TextStatus::kOk, FormatDateTime(value, "", buffer, 1).status);
  EXPECT_EQ('\0', buffer[0]);
  EXPECT_EQ(TextStatus::kInvalidInput,
            FormatDateTime(value, "%F", nullptr, 1).status);
}

TEST(FormatDateTime, InvalidFormatsAndEmptyStrings) {
  for (const char* format : {"%", "%Q", "%:Y", "%:", "%Z", "%c", "%s", "%3f"}) {
    char buffer[64] = "unchanged";
    auto result = FormatDateTime(DateTime(), format, buffer, sizeof(buffer));
    EXPECT_EQ(TextStatus::kInvalidFormat, result.status) << format;
    EXPECT_EQ(0u, result.size);
    EXPECT_STREQ("", buffer);
    EXPECT_EQ("", FormatDateTime(DateTime(), format));
    DateTime output;
    EXPECT_EQ(TextStatus::kInvalidFormat,
              ParseDateTime("", format, timezone::UTC, &output).status);
  }
  EXPECT_EQ("", FormatDateTime(DateTime(), ""));
  EXPECT_EQ("", FormatDateTime(DateTime(), static_cast<const char*>(nullptr)));
  char buffer[10];
  EXPECT_EQ(
      TextStatus::kOk,
      FormatDateTime(DateTime(), nullptr, 0, buffer, sizeof(buffer)).status);
  EXPECT_EQ(
      TextStatus::kInvalidFormat,
      FormatDateTime(DateTime(), "x\0y", 3, buffer, sizeof(buffer)).status);
}

TEST(FormatDateTime, ViewsAreBoundedAndNeedNoTerminator) {
  const char format[] = {'%', 'F'};
  const char input[] = {'2', '0', '2', '4', '-', '0', '2', '-', '2', '9'};
  roo::string_view format_view(format, sizeof(format));
  roo::string_view input_view(input, sizeof(input));
  DateTime output;
  auto parsed = ParseDateTime(input_view, format_view, timezone::UTC, &output);
  ASSERT_EQ(TextStatus::kOk, parsed.status);
  EXPECT_EQ(sizeof(input), parsed.position);
  EXPECT_EQ("2024-02-29", FormatDateTime(output, format_view));
  char buffer[11];
  EXPECT_EQ(TextStatus::kOk,
            FormatDateTime(output, format_view, buffer, sizeof(buffer)).status);
  EXPECT_STREQ("2024-02-29", buffer);
  EXPECT_EQ(
      TextStatus::kInvalidInput,
      ParseDateTime(roo::string_view(), format_view, timezone::UTC, &output)
          .status);
  EXPECT_EQ("", FormatDateTime(output, roo::string_view()));
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseDateTime(roo::string_view("2024-02-29\0", 11), format_view,
                          timezone::UTC, &output)
                .status);
}

TEST(ParseDateTime, RoundTripsCalendarBoundariesAndOffsets) {
  for (int year :
       {1, 4, 100, 400, 1600, 1900, 1969, 1970, 2000, 2024, 2038, 9999}) {
    for (int month = 1; month <= 12; ++month) {
      for (int offset : {-1439, -330, -1, 0, 1, 345, 1439}) {
        for (int micros : {0, 1, 100000, 999999}) {
          DateTime value(year, month, 1, 23, 59, 59, micros,
                         UtcOffset(Minutes(offset)));
          for (const char* format : {kFormat, "%Y/%m/%d %T.%f %z"}) {
            std::string text = FormatDateTime(value, format);
            DateTime parsed;
            ASSERT_EQ(
                TextStatus::kOk,
                ParseDateTime(text, format, timezone::UTC, &parsed).status)
                << text;
            EXPECT_EQ(value, parsed) << text;
          }
        }
      }
    }
  }
}

TEST(ParseDateTime, FractionsAndDefaults) {
  DateTime output;
  EXPECT_EQ(
      TextStatus::kOk,
      ParseDateTime("2024-02-29", "%F", UtcOffset(Hours(3)), &output).status);
  EXPECT_EQ(DateTime(2024, 2, 29, UtcOffset(Hours(3))), output);
  for (int width = 1; width <= 6; ++width) {
    std::string text = "2024-02-29 12:34:56." + std::string(width, '1');
    ASSERT_EQ(TextStatus::kOk,
              ParseDateTime(text, "%F %T.%f", timezone::UTC, &output).status);
    unsigned expected = 0;
    for (int i = 0; i < 6; ++i) expected = expected * 10 + (i < width ? 1 : 0);
    EXPECT_EQ(expected, output.micros());
  }
  ASSERT_EQ(
      TextStatus::kOk,
      ParseDateTime("2024-02-29 08", "%F %H", timezone::UTC, &output).status);
  EXPECT_EQ(8, output.hour());
  EXPECT_EQ(0, output.minute());
  EXPECT_EQ(0u, output.micros());
}

TEST(ParseDateTime, OffsetOverridesAndRepeatedFields) {
  DateTime output;
  for (const char* format : {"%F%z", "%F%:z"}) {
    ASSERT_EQ(TextStatus::kOk,
              ParseDateTime("2024-02-29Z", format, UtcOffset(Hours(3)), &output)
                  .status);
    EXPECT_EQ(0, output.timeZone().offset().inMinutes());
  }
  EXPECT_EQ(TextStatus::kOk,
            ParseDateTime("2024-02-29-00:30", "%F%:z", timezone::UTC, &output)
                .status);
  EXPECT_EQ(-30, output.timeZone().offset().inMinutes());
  EXPECT_EQ(TextStatus::kOk,
            ParseDateTime("2024-02-29/2024 .1/.100000 +0100/+01:00",
                          "%F/%Y .%f/.%f %z/%:z", timezone::UTC, &output)
                .status);
  EXPECT_EQ(
      TextStatus::kInvalidInput,
      ParseDateTime("2024-02-29/2025", "%F/%Y", timezone::UTC, &output).status);
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseDateTime("2024-02-29+0100/+02:00", "%F%z/%:z", timezone::UTC,
                          &output)
                .status);
}

TEST(ParseDateTime, StrictValidationAndUnchangedDestination) {
  const DateTime original(2001, 1, 2, UtcOffset(Hours(1)));
  struct Case {
    const char* text;
    const char* format;
    TextStatus status;
  };
  const Case cases[] = {
      {"0000-01-01", "%F", TextStatus::kOutOfRange},
      {"10000-01-01", "%F", TextStatus::kInvalidInput},
      {"1900-02-29", "%F", TextStatus::kOutOfRange},
      {"2023-02-29", "%F", TextStatus::kOutOfRange},
      {"2024-04-31", "%F", TextStatus::kOutOfRange},
      {"2024-00-01", "%F", TextStatus::kOutOfRange},
      {"2024-13-01", "%F", TextStatus::kOutOfRange},
      {"2024-01-00", "%F", TextStatus::kOutOfRange},
      {"2024-01-32", "%F", TextStatus::kOutOfRange},
      {"2024-01-01 24:00:00", "%F %T", TextStatus::kOutOfRange},
      {"2024-01-01 23:60:00", "%F %T", TextStatus::kOutOfRange},
      {"2024-01-01 23:59:60", "%F %T", TextStatus::kOutOfRange},
      {"2024-01-01.1234567", "%F.%f", TextStatus::kOutOfRange},
      {"2024-01-01.", "%F.%f", TextStatus::kInvalidInput},
      {"2024-01-01+24:00", "%F%:z", TextStatus::kOutOfRange},
      {"2024-01-01+12:60", "%F%:z", TextStatus::kOutOfRange},
      {"2024-01-01+0100", "%F%:z", TextStatus::kInvalidInput},
      {"2024-01-01+01:00", "%F%z", TextStatus::kInvalidInput},
      {"2024-1-01", "%F", TextStatus::kInvalidInput},
      {"2024-01-01 ", "%F", TextStatus::kInvalidInput},
      {" 2024-01-01", "%F", TextStatus::kInvalidInput},
      {"2024-01-0", "%F", TextStatus::kInvalidInput},
      {"2024-01", "%Y-%m", TextStatus::kInvalidInput},
      {"12:30:00", "%T", TextStatus::kInvalidInput},
  };
  for (const auto& c : cases) {
    DateTime output = original;
    auto result = ParseDateTime(c.text, c.format, timezone::UTC, &output);
    EXPECT_EQ(c.status, result.status) << c.text;
    EXPECT_LE(result.position, std::strlen(c.text));
    EXPECT_EQ(original, output);
  }
  DateTime output = original;
  EXPECT_EQ(8u,
            ParseDateTime("2023-02-29", "%F", timezone::UTC, &output).position);
  EXPECT_EQ(5u,
            ParseDateTime("2024-x1-01", "%F", timezone::UTC, &output).position);
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseDateTime(nullptr, 1, "%F", timezone::UTC, &output).status);
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseDateTime("2024-01-01", "%F", timezone::UTC, nullptr).status);
}

TEST(FormatDateTime, OffsetsOutsideTextRange) {
  for (int offset : {-32768, -1440, 1440, 32767}) {
    DateTime value(2024, 1, 1, UtcOffset(Minutes(offset)));
    char buffer[40] = "old";
    EXPECT_EQ(TextStatus::kOutOfRange,
              FormatDateTime(value, kFormat, buffer, sizeof(buffer)).status);
    EXPECT_STREQ("", buffer);
    EXPECT_EQ("", FormatDateTime(value, kFormat));
    EXPECT_EQ("2024-01-01", FormatDateTime(value, "%F"));
    DateTime output;
    EXPECT_EQ(
        TextStatus::kOk,
        ParseDateTime("2024-01-01", "%F", value.timeZone(), &output).status);
    EXPECT_EQ(value, output);
  }
}

}  // namespace
}  // namespace roo_time
