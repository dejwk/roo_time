#include <cstring>

#include "gtest/gtest.h"
#include "roo_time/format.h"

namespace roo_time {
namespace {

// Verifies numeric layouts round-trip at boundaries and leap centuries.
TEST(CivilDayText, RoundTrip) {
  for (CivilDay date :
       {CivilDay::FromYmd(1, 1, 1), CivilDay::FromYmd(9999, 12, 31),
        CivilDay::FromYmd(1900, 3, 1), CivilDay::FromYmd(2000, 2, 29),
        CivilDay::FromYmd(1969, 12, 31)}) {
    for (const char* format : {"%F", "%d.%m.%Y", "%m/%d/%Y", "%F %% %Y"}) {
      char buffer[32];
      const FormatResult formatted =
          FormatCivilDay(date, format, buffer, sizeof(buffer));
      ASSERT_EQ(TextStatus::kOk, formatted.status);
      EXPECT_EQ(std::strlen(buffer), formatted.size);
      CivilDay parsed;
      const ParseResult result =
          ParseCivilDay(buffer, std::strlen(buffer), format, &parsed);
      EXPECT_EQ(TextStatus::kOk, result.status);
      EXPECT_EQ(std::strlen(buffer), result.position);
      EXPECT_EQ(date, parsed);
      char datetime[32];
      ASSERT_EQ(TextStatus::kOk,
                FormatDateTime(DateTime(date.year(), date.month(), date.day(),
                                        timezone::UTC),
                               format, datetime, sizeof(datetime))
                    .status);
      EXPECT_STREQ(datetime, buffer);
    }
  }
}

// Verifies rejected input leaves the destination intact and reports its offset.
TEST(CivilDayText, StrictInput) {
  const CivilDay original = CivilDay::FromYmd(2024, 1, 1);
  struct Case {
    const char* text;
    const char* format;
    TextStatus status;
    size_t position;
  };
  const Case cases[] = {
      {"1900-02-29", "%F", TextStatus::kOutOfRange, 8},
      {"2024-04-31", "%F", TextStatus::kOutOfRange, 8},
      {"0000-01-01", "%F", TextStatus::kOutOfRange, 0},
      {"2024-13-01", "%F", TextStatus::kOutOfRange, 5},
      {"2024-00-01", "%F", TextStatus::kOutOfRange, 5},
      {"2024-01-00", "%F", TextStatus::kOutOfRange, 8},
      {"2024-1-01", "%F", TextStatus::kInvalidInput, 6},
      {"2024-01-", "%F", TextStatus::kInvalidInput, 8},
      {"2024-01-01x", "%F", TextStatus::kInvalidInput, 10},
      {"2024-01-01/2025", "%F/%Y", TextStatus::kInvalidInput, 11},
      {"2024-01", "%Y-%m", TextStatus::kInvalidInput, 7},
      {"", "%F", TextStatus::kInvalidInput, 0},
  };
  for (const Case& c : cases) {
    CivilDay parsed = original;
    const ParseResult result =
        ParseCivilDay(c.text, std::strlen(c.text), c.format, &parsed);
    EXPECT_EQ(c.status, result.status) << c.text;
    EXPECT_EQ(c.position, result.position) << c.text;
    EXPECT_EQ(original, parsed);
  }
}

// Verifies date-only APIs reject time/zone directives, including expanded
// aliases.
TEST(CivilDayText, RejectsTimeFormats) {
  for (const char* format :
       {"%F%H", "%M", "%S", "%T", "%f", "%z", "%:z", "%Q", "%"}) {
    CivilDay parsed = CivilDay::FromYmd(2024, 1, 1);
    const CivilDay original = parsed;
    EXPECT_EQ(TextStatus::kInvalidFormat,
              ParseCivilDay("", 0, format, &parsed).status);
    EXPECT_EQ(original, parsed);
    char buffer[16] = "previous";
    EXPECT_EQ(TextStatus::kInvalidFormat,
              FormatCivilDay(parsed, format, buffer, sizeof(buffer)).status);
    EXPECT_STREQ("", buffer);
  }
}

// Verifies size queries, truncation, invalid values, and null pointer
// contracts.
TEST(CivilDayText, BufferAndInvalidContracts) {
  const CivilDay date = CivilDay::FromYmd(1, 1, 1);
  char buffer[16];
  for (size_t capacity = 0; capacity <= 11; ++capacity) {
    std::memset(buffer, '!', sizeof(buffer));
    const FormatResult result = FormatCivilDay(date, "%F", buffer, capacity);
    EXPECT_EQ(10u, result.size);
    EXPECT_EQ(capacity > 10 ? TextStatus::kOk : TextStatus::kBufferTooSmall,
              result.status);
    EXPECT_EQ('!', buffer[capacity]);
    if (capacity != 0) {
      EXPECT_EQ('\0', buffer[capacity - 1]);
      EXPECT_EQ(0, std::memcmp(buffer, "0001-01-01", capacity - 1));
    }
  }
  EXPECT_EQ(10u, FormatCivilDay(date, "%F", nullptr, 0).size);
  EXPECT_EQ(TextStatus::kBufferTooSmall,
            FormatCivilDay(date, "%F", nullptr, 0).status);
  EXPECT_EQ(TextStatus::kInvalidInput,
            FormatCivilDay(date, "%F", nullptr, 1).status);
  const FormatResult invalid =
      FormatCivilDay(CivilDay(), "%F", buffer, sizeof(buffer));
  EXPECT_EQ(TextStatus::kInvalidInput, invalid.status);
  EXPECT_EQ(0u, invalid.size);
  EXPECT_STREQ("", buffer);
  EXPECT_EQ(TextStatus::kInvalidFormat,
            FormatCivilDay(date, nullptr, buffer, sizeof(buffer)).status);
  EXPECT_EQ(TextStatus::kOk,
            FormatCivilDay(date, nullptr, 0, buffer, sizeof(buffer)).status);
  CivilDay parsed = date;
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseCivilDay(nullptr, 1, "%F", &parsed).status);
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseCivilDay(nullptr, 0, "%F", &parsed).status);
  EXPECT_EQ(TextStatus::kInvalidInput,
            ParseCivilDay("0001-01-01", 10, "%F", nullptr).status);
  EXPECT_EQ(TextStatus::kInvalidFormat,
            ParseCivilDay("0001-01-01", 10, nullptr, &parsed).status);
  EXPECT_EQ(date, parsed);
}

// Verifies bounded views neither require a terminator nor consume trailing
// data.
TEST(CivilDayText, BoundedViews) {
  const char text[] = {'2', '0', '2', '4', '-', '0', '2', '-', '2', '9', 'x'};
  const char format[] = {'%', 'F', 'x'};
  CivilDay parsed;
  EXPECT_EQ(TextStatus::kOk, ParseCivilDay(roo::string_view(text, 10),
                                           roo::string_view(format, 2), &parsed)
                                 .status);
  char buffer[11];
  EXPECT_EQ(TextStatus::kOk, FormatCivilDay(parsed, roo::string_view(format, 2),
                                            buffer, sizeof(buffer))
                                 .status);
  EXPECT_STREQ("2024-02-29", buffer);
}

}  // namespace
}  // namespace roo_time
