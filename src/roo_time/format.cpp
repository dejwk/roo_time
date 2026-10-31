// Only the inline convenience overloads need the STL/backport headers.
#undef ROO_TIME_HAS_STD_STRING
#undef ROO_TIME_HAS_STRING_VIEW
#define ROO_TIME_HAS_STD_STRING 0
#define ROO_TIME_HAS_STRING_VIEW 0
#include "roo_time/format.h"

// Override at compile time to force the portable backend or enable strftime on
// another toolchain known to provide <ctime> and std::strftime.
#ifndef ROO_TIME_HAS_STRFTIME
#if defined(__linux__) || defined(ESP_PLATFORM)
#define ROO_TIME_HAS_STRFTIME 1
#else
#define ROO_TIME_HAS_STRFTIME 0
#endif
#endif
#if ROO_TIME_HAS_STRFTIME
#include <ctime>
#endif

namespace roo_time {
namespace {

bool IsDigit(char c) { return c >= '0' && c <= '9'; }

// Expands the two aliases without allocation or recursion. A zero code denotes
// a literal; every other code is a validated directive (':' represents %:z).
class Tokens {
 public:
  Tokens(const char* data, size_t size) : data_(data), size_(size) {}

  bool nextToken(char& code, char& literal) {
    char c;
    if (!getChar(c)) return false;
    code = '\0';
    literal = c;
    if (c == '\0') {
      valid_ = false;
    } else if (c == '%') {
      if (!getChar(code)) {
        valid_ = false;
      } else if (code == 'F' || code == 'T') {
        alias_ = code == 'F' ? "%Y-%m-%d" : "%H:%M:%S";
        return nextToken(code, literal);
      } else if (code == '%') {
        code = '\0';
        literal = '%';
      } else if (code == ':') {
        char z;
        if (!getChar(z) || z != 'z') valid_ = false;
      } else if (code == '\0' || std::strchr("YmdHMSfz", code) == nullptr) {
        valid_ = false;
      }
    }
    return valid_;
  }

  bool isValid() const { return valid_; }

 private:
  bool getChar(char& c) {
    if (alias_ != nullptr) {
      if (*alias_ != '\0') {
        c = *alias_++;
        return true;
      }
      alias_ = nullptr;
    }
    if (pos_ == size_) return false;
    c = data_[pos_++];
    return true;
  }

  const char* data_;
  size_t size_;
  size_t pos_ = 0;
  const char* alias_ = nullptr;
  bool valid_ = true;
};

enum class FieldSet { kDateTime, kDate, kTime };

bool ValidFormat(const char* format, size_t length, bool& has_offset,
                 FieldSet field_set) {
  if (format == nullptr && length != 0) return false;
  Tokens tokens(format, length);
  char code, literal;
  has_offset = false;
  while (tokens.nextToken(code, literal)) {
    if (code != '\0' && ((field_set == FieldSet::kDate &&
                          std::strchr("Ymd", code) == nullptr) ||
                         (field_set == FieldSet::kTime &&
                          std::strchr("HMSf", code) == nullptr))) {
      return false;
    }
    if (code == 'z' || code == ':') has_offset = true;
  }
  return tokens.isValid();
}

// Accumulates a bounded, NUL-terminated formatted result while measuring it.
class Writer {
 public:
  Writer(char* buffer, size_t capacity)
      : buffer_(buffer), capacity_(capacity) {}

  void putChar(char c) {
    // Saturate rather than wrap on targets with a small size_t. SIZE_MAX bytes
    // cannot be represented together with the required terminating NUL.
    if (size_ == static_cast<size_t>(-1)) return;
    if (capacity_ != 0 && size_ < capacity_ - 1) buffer_[size_] = c;
    ++size_;
  }

  void putNumber(uint32_t value, unsigned width) {
    char digits[6];
    for (unsigned i = width; i > 0; --i) {
      digits[i - 1] = '0' + value % 10;
      value /= 10;
    }
    for (unsigned i = 0; i < width; ++i) putChar(digits[i]);
  }

  // Writes a full unsigned count with minimum-width padding, never truncating.
  void putUnsigned(uint64_t value, unsigned width, char padding) {
    char digits[20];
    unsigned count = 0;
    do {
      digits[count++] = '0' + value % 10;
      value /= 10;
    } while (value != 0);
    while (width > count) {
      putChar(padding);
      --width;
    }
    while (count != 0) putChar(digits[--count]);
  }

  FormatResult finishResult() {
    if (size_ == static_cast<size_t>(-1)) {
      if (capacity_ != 0) buffer_[0] = '\0';
      return {TextStatus::kOutOfRange, 0};
    }
    if (capacity_ != 0)
      buffer_[size_ < capacity_ ? size_ : capacity_ - 1] = '\0';
    return {size_ < capacity_ ? TextStatus::kOk : TextStatus::kBufferTooSmall,
            size_};
  }

 private:
  char* buffer_;
  size_t capacity_;
  size_t size_ = 0;
};

// Duration directives have their own grammar; calendar aliases stay unchanged.
struct DurationToken {
  char code = '\0';
  char literal = '\0';
  char padding = '0';
  unsigned width = 0;
  unsigned precision = 6;
};

// Reads one bounded duration token, including width or fractional precision.
class DurationTokens {
 public:
  DurationTokens(const char* data, size_t size) : data_(data), size_(size) {}

  bool nextToken(DurationToken& token) {
    if (pos_ == size_ || !valid_) return false;
    token = DurationToken();
    const char c = data_[pos_++];
    if (c == '\0') return fail();
    if (c != '%') {
      token.literal = c;
      return true;
    }
    if (pos_ == size_) return fail();
    if (data_[pos_] == '.') {
      ++pos_;
      if (pos_ == size_ || data_[pos_] < '1' || data_[pos_] > '6')
        return fail();
      token.precision = data_[pos_++] - '0';
      if (pos_ == size_ || data_[pos_++] != 'f') return fail();
      token.code = 'f';
      return true;
    }
    bool modified = false;
    if (data_[pos_] == '-') {
      modified = true;
      ++pos_;
    } else if (IsDigit(data_[pos_])) {
      modified = true;
      token.padding = data_[pos_] == '0' ? '0' : ' ';
      do {
        token.width = token.width * 10 + data_[pos_++] - '0';
        if (token.width > 255) return fail();
      } while (pos_ < size_ && IsDigit(data_[pos_]));
      if (token.width == 0) return fail();
    }
    if (pos_ == size_) return fail();
    token.code = data_[pos_++];
    if (token.code != '\0' && std::strchr("DHhMS", token.code) != nullptr) {
      if (!modified) token.width = token.code == 'D' ? 1 : 2;
      return true;
    }
    if (modified) return fail();
    if (token.code == '%') {
      token.code = '\0';
      token.literal = '%';
      return true;
    }
    if (token.code == 'f' || token.code == 'F' || token.code == 'T' ||
        token.code == '+')
      return true;
    return fail();
  }

  bool isValid() const { return valid_; }

 private:
  bool fail() {
    valid_ = false;
    return false;
  }

  const char* data_;
  size_t size_;
  size_t pos_ = 0;
  bool valid_ = true;
};

// Emits the shortest clock with trailing components padded to two digits.
void WriteCompactDuration(Writer& writer, uint64_t magnitude) {
  const uint64_t hours = magnitude / 3600000000LL;
  const unsigned minutes = magnitude / 60000000LL % 60;
  const unsigned seconds = magnitude / 1000000LL % 60;
  if (hours != 0) {
    writer.putUnsigned(hours, 1, '0');
    writer.putChar(':');
    writer.putNumber(minutes, 2);
    writer.putChar(':');
    writer.putNumber(seconds, 2);
  } else if (minutes != 0) {
    writer.putUnsigned(minutes, 1, '0');
    writer.putChar(':');
    writer.putNumber(seconds, 2);
  } else {
    writer.putUnsigned(seconds, 1, '0');
  }
}

// Emits a truncated fixed fraction or a decimal-prefixed trimmed fraction.
void WriteDurationFraction(Writer& writer, uint32_t micros,
                           DurationToken token) {
  if (token.code == 'F') {
    if (micros == 0) return;
    writer.putChar('.');
    while (micros % 10 == 0) {
      micros /= 10;
      --token.precision;
    }
  } else {
    for (unsigned digits = token.precision; digits < 6; ++digits) micros /= 10;
  }
  writer.putNumber(micros, token.precision);
}

// Writes a validated directive using the exact unsigned duration magnitude.
void WriteDurationToken(Writer& writer, const DurationToken& token,
                        uint64_t magnitude) {
  uint64_t value = 0;
  switch (token.code) {
    case '\0':
      writer.putChar(token.literal);
      return;
    case 'T':
      WriteCompactDuration(writer, magnitude);
      return;
    case 'f':
    case 'F':
      WriteDurationFraction(writer, magnitude % 1000000LL, token);
      return;
    case 'D':
      value = magnitude / 86400000000LL;
      break;
    case 'H':
      value = magnitude / 3600000000LL;
      break;
    case 'h':
      value = magnitude / 3600000000LL % 24;
      break;
    case 'M':
      value = magnitude / 60000000LL % 60;
      break;
    case 'S':
      value = magnitude / 1000000LL % 60;
      break;
  }
  writer.putUnsigned(value, token.width, token.padding);
}

enum Field {
  kYear,
  kMonth,
  kDay,
  kHour,
  kMinute,
  kSecond,
  kMicros,
  kOffset,
  kCount
};

// Formats a fixed-width calendar field, using strftime only when it agrees.
void CalendarNumber(Writer& writer, const int32_t* value, char code,
                    unsigned number) {
#if ROO_TIME_HAS_STRFTIME
  std::tm calendar = {};
  calendar.tm_year = value[kYear] - 1900;
  calendar.tm_mon = value[kMonth] - 1;
  calendar.tm_mday = value[kDay];
  calendar.tm_hour = value[kHour];
  calendar.tm_min = value[kMinute];
  calendar.tm_sec = value[kSecond];
  calendar.tm_isdst = -1;
  const char format[] = {'%', code, '\0'};
  char text[3];
  // Only fixed-width numeric fields are delegated. Check the result so locale
  // or libc quirks cannot change the portable output. Years, fractions and
  // offsets always use the portable path.
  if (std::strftime(text, sizeof(text), format, &calendar) == 2 &&
      IsDigit(text[0]) && IsDigit(text[1]) &&
      static_cast<unsigned>((text[0] - '0') * 10 + text[1] - '0') == number) {
    writer.putChar(text[0]);
    writer.putChar(text[1]);
    return;
  }
#else
  (void)value;
  (void)code;
#endif
  writer.putNumber(number, 2);
}

// Tracks strict format parsing and prevents inconsistent duplicate fields.
class Parser {
 public:
  Parser(const char* text, size_t length) : text_(text), length_(length) {}

  bool matchLiteral(char c) {
    if (pos == length_ || text_[pos] != c)
      return failWith(TextStatus::kInvalidInput);
    ++pos;
    return true;
  }

  bool parseNumber(unsigned width, int32_t& value) {
    value = 0;
    for (unsigned i = 0; i < width; ++i) {
      if (!hasDigit()) return failWith(TextStatus::kInvalidInput);
      value = value * 10 + text_[pos++] - '0';
    }
    return true;
  }

  bool parseDirective(char code) {
    const size_t start = pos;
    int32_t value = 0;
    Field field;
    int minimum = 0, maximum = 59;
    unsigned width = 2;
    switch (code) {
      case 'Y':
        field = kYear;
        width = 4;
        minimum = 1;
        maximum = 9999;
        break;
      case 'm':
        field = kMonth;
        minimum = 1;
        maximum = 12;
        break;
      case 'd':
        field = kDay;
        minimum = 1;
        maximum = 31;
        break;
      case 'H':
        field = kHour;
        maximum = 23;
        break;
      case 'M':
        field = kMinute;
        break;
      case 'S':
        field = kSecond;
        break;
      case 'f': {
        unsigned digits = 0;
        while (digits < 6 && hasDigit()) {
          value = value * 10 + text_[pos++] - '0';
          ++digits;
        }
        if (digits == 0) return failWith(TextStatus::kInvalidInput);
        while (digits++ < 6) value *= 10;
        return setField(kMicros, value, start);
      }
      default: {
        // Both offset directives accept the UTC designator.
        if (pos < length_ && text_[pos] == 'Z') {
          ++pos;
          return setField(kOffset, 0, start);
        }
        if (pos == length_ || (text_[pos] != '+' && text_[pos] != '-')) {
          return failWith(TextStatus::kInvalidInput);
        }
        const bool negative = text_[pos++] == '-';
        int32_t hours, minutes;
        if (!parseNumber(2, hours)) return false;
        if (code == ':' && !matchLiteral(':')) return false;
        if (!parseNumber(2, minutes)) return false;
        if (hours > 23 || minutes > 59)
          return failWith(TextStatus::kOutOfRange, start);
        value = hours * 60 + minutes;
        return setField(kOffset, negative ? -value : value, start);
      }
    }
    if (!parseNumber(width, value)) return false;
    if (value < minimum || value > maximum)
      return failWith(TextStatus::kOutOfRange, start);
    return setField(field, value, start);
  }

  bool isComplete(FieldSet field_set) {
    if (pos != length_) return failWith(TextStatus::kInvalidInput);
    if (field_set == FieldSet::kTime) {
      return (seen[kHour] && seen[kMinute]) ||
             failWith(TextStatus::kInvalidInput);
    }
    if (!seen[kYear] || !seen[kMonth] || !seen[kDay]) {
      return failWith(TextStatus::kInvalidInput);
    }
    const int max_day =
        DaysInMonth(fields[kYear], static_cast<Month>(fields[kMonth]));
    if (fields[kDay] > max_day)
      return failWith(TextStatus::kOutOfRange, positions[kDay]);
    return true;
  }

  size_t pos = 0;
  TextStatus status = TextStatus::kOk;
  int32_t fields[kCount] = {};
  bool seen[kCount] = {};

 private:
  bool hasDigit() const { return pos < length_ && IsDigit(text_[pos]); }

  bool failWith(TextStatus error) {
    status = error;
    return false;
  }

  bool failWith(TextStatus error, size_t position) {
    pos = position;
    return failWith(error);
  }

  bool setField(Field field, int32_t value, size_t start) {
    if (seen[field] && fields[field] != value)
      return failWith(TextStatus::kInvalidInput, start);
    fields[field] = value;
    seen[field] = true;
    positions[field] = start;
    return true;
  }
  const char* text_;
  size_t length_;
  size_t positions[kCount] = {};
};

// Formats numeric fields for dates, times of day, and date-times.
FormatResult FormatFields(const int32_t* value, const char* format,
                          size_t format_length, char* buffer, size_t capacity,
                          FieldSet field_set) {
  if (capacity != 0 && buffer == nullptr) return {TextStatus::kInvalidInput, 0};
  if (capacity != 0) buffer[0] = '\0';
  bool has_offset;
  if (!ValidFormat(format, format_length, has_offset, field_set))
    return {TextStatus::kInvalidFormat, 0};
  const int offset = value[kOffset];
  if (has_offset && (offset < -1439 || offset > 1439))
    return {TextStatus::kOutOfRange, 0};

  Writer writer(buffer, capacity);
  Tokens tokens(format, format_length);
  char code, literal;
  while (tokens.nextToken(code, literal)) {
    switch (code) {
      case '\0':
        writer.putChar(literal);
        break;
      case 'Y':
        writer.putNumber(value[kYear], 4);
        break;
      case 'm':
        CalendarNumber(writer, value, code, value[kMonth]);
        break;
      case 'd':
        CalendarNumber(writer, value, code, value[kDay]);
        break;
      case 'H':
        CalendarNumber(writer, value, code, value[kHour]);
        break;
      case 'M':
        CalendarNumber(writer, value, code, value[kMinute]);
        break;
      case 'S':
        CalendarNumber(writer, value, code, value[kSecond]);
        break;
      case 'f':
        writer.putNumber(value[kMicros], 6);
        break;
      default: {
        writer.putChar(offset < 0 ? '-' : '+');
        const unsigned magnitude = offset < 0 ? -offset : offset;
        writer.putNumber(magnitude / 60, 2);
        if (code == ':') writer.putChar(':');
        writer.putNumber(magnitude % 60, 2);
        break;
      }
    }
  }
  return writer.finishResult();
}

// Parses into shared fields after validating the format and destination.
ParseResult ParseFields(const char* text, size_t length, const char* format,
                        size_t format_length, FieldSet field_set,
                        const void* result, Parser& parser) {
  bool has_offset;
  if (!ValidFormat(format, format_length, has_offset, field_set))
    return {TextStatus::kInvalidFormat, 0};
  if ((text == nullptr && length != 0) || result == nullptr)
    return {TextStatus::kInvalidInput, 0};
  Tokens tokens(format, format_length);
  char code, literal;
  while (tokens.nextToken(code, literal)) {
    if (!(code != '\0' ? parser.parseDirective(code)
                       : parser.matchLiteral(literal))) {
      return {parser.status, parser.pos};
    }
  }
  if (!parser.isComplete(field_set)) return {parser.status, parser.pos};
  return {TextStatus::kOk, parser.pos};
}

}  // namespace

FormatResult FormatDuration(Duration value, const char* format,
                            size_t format_length, char* buffer,
                            size_t capacity) {
  if (capacity != 0 && buffer == nullptr) return {TextStatus::kInvalidInput, 0};
  if (capacity != 0) buffer[0] = '\0';
  if (format == nullptr && format_length != 0)
    return {TextStatus::kInvalidFormat, 0};
  DurationToken token;
  DurationTokens validation(format, format_length);
  bool explicit_sign = false;
  while (validation.nextToken(token)) {
    if (token.code == '+') explicit_sign = true;
  }
  if (!validation.isValid()) return {TextStatus::kInvalidFormat, 0};

  const int64_t micros = value.inMicros();
  const uint64_t magnitude =
      micros < 0 ? uint64_t{0} - uint64_t(micros) : uint64_t(micros);
  bool needs_sign = micros < 0 && !explicit_sign;
  Writer writer(buffer, capacity);
  DurationTokens tokens(format, format_length);
  while (tokens.nextToken(token)) {
    if (token.code == '+') {
      writer.putChar(micros < 0 ? '-' : '+');
      continue;
    }
    // A trimmed zero fraction emits nothing, including no orphan minus sign.
    if (token.code == 'F' && magnitude % 1000000LL == 0) continue;
    if (needs_sign) {
      writer.putChar('-');
      needs_sign = false;
    }
    WriteDurationToken(writer, token, magnitude);
  }
  return writer.finishResult();
}

FormatResult FormatUptime(Uptime value, const char* format,
                          size_t format_length, char* buffer, size_t capacity) {
  return FormatDuration(value - Uptime::Start(), format, format_length, buffer,
                        capacity);
}

FormatResult FormatDateTime(const DateTime& value, const char* format,
                            size_t format_length, char* buffer,
                            size_t capacity) {
  const int32_t fields[kCount] = {
      value.year(),
      value.month(),
      value.day(),
      value.hour(),
      value.minute(),
      value.second(),
      static_cast<int32_t>(value.micros()),
      static_cast<int32_t>(value.utcOffset().inMinutes())};
  return FormatFields(fields, format, format_length, buffer, capacity,
                      FieldSet::kDateTime);
}

FormatResult FormatCivilDay(CivilDay value, const char* format,
                            size_t format_length, char* buffer,
                            size_t capacity) {
  if (!value.isValid()) {
    if (capacity != 0 && buffer != nullptr) buffer[0] = '\0';
    return {TextStatus::kInvalidInput, 0};
  }
  const int32_t fields[kCount] = {value.year(), value.month(), value.day()};
  return FormatFields(fields, format, format_length, buffer, capacity,
                      FieldSet::kDate);
}

FormatResult FormatTimeOfDay(TimeOfDay value, const char* format,
                             size_t format_length, char* buffer,
                             size_t capacity) {
  if (!value.isValid()) {
    if (capacity != 0 && buffer != nullptr) buffer[0] = '\0';
    return {TextStatus::kInvalidInput, 0};
  }
  // Supply a valid neutral date for libc backends; date directives are
  // rejected.
  const int32_t fields[kCount] = {2000,
                                  1,
                                  1,
                                  value.hour(),
                                  value.minute(),
                                  value.second(),
                                  static_cast<int32_t>(value.micros()),
                                  0};
  return FormatFields(fields, format, format_length, buffer, capacity,
                      FieldSet::kTime);
}

ParseResult ParseTimeOfDay(const char* text, size_t length, const char* format,
                           size_t format_length, TimeOfDay* result) {
  Parser parser(text, length);
  const ParseResult parsed = ParseFields(text, length, format, format_length,
                                         FieldSet::kTime, result, parser);
  if (parsed.status == TextStatus::kOk) {
    *result =
        TimeOfDay::FromHms(parser.fields[kHour], parser.fields[kMinute],
                           parser.fields[kSecond], parser.fields[kMicros]);
  }
  return parsed;
}

ParseResult ParseDateTime(const char* text, size_t length, const char* format,
                          size_t format_length, UtcOffset default_offset,
                          DateTime* result) {
  Parser parser(text, length);
  const ParseResult parsed = ParseFields(text, length, format, format_length,
                                         FieldSet::kDateTime, result, parser);
  if (parsed.status != TextStatus::kOk) return parsed;
  const int32_t* f = parser.fields;
  *result = DateTime(
      f[kYear], f[kMonth], f[kDay], f[kHour], f[kMinute], f[kSecond],
      f[kMicros],
      parser.seen[kOffset] ? UtcOffset(Minutes(f[kOffset])) : default_offset);
  return parsed;
}

ParseResult ParseCivilDay(const char* text, size_t length, const char* format,
                          size_t format_length, CivilDay* result) {
  Parser parser(text, length);
  const ParseResult parsed = ParseFields(text, length, format, format_length,
                                         FieldSet::kDate, result, parser);
  if (parsed.status == TextStatus::kOk) {
    *result = CivilDay::FromYmd(parser.fields[kYear], parser.fields[kMonth],
                                parser.fields[kDay]);
  }
  return parsed;
}

ParseResult ParseIsoDateTime(const char* text, size_t length,
                             DateTime* result) {
  if (text == nullptr && length != 0) return {TextStatus::kInvalidInput, 0};
  // The complete fixed-width date and time occupy the first 19 bytes. Selecting
  // the fractional form never reads beyond a view or needs a temporary string.
  const char* format =
      length > 19 && text[19] == '.' ? kIsoDateTimeFormat : "%FT%T%:z";
  return ParseDateTime(text, length, format, timezone::UTC, result);
}

}  // namespace roo_time
