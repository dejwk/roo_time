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

bool ValidFormat(const char* format, size_t length, bool& has_offset,
                 bool date_only = false) {
  if (format == nullptr && length != 0) return false;
  Tokens tokens(format, length);
  char code, literal;
  has_offset = false;
  while (tokens.nextToken(code, literal)) {
    if (date_only && code != '\0' && code != 'Y' && code != 'm' && code != 'd')
      return false;
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
        if (hasDigit()) return failWith(TextStatus::kOutOfRange);
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

  bool isComplete() {
    if (pos != length_ || !seen[kYear] || !seen[kMonth] || !seen[kDay]) {
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

// Formats shared numeric fields for either a civil date or a date-time.
FormatResult FormatFields(const int32_t* value, const char* format,
                          size_t format_length, char* buffer, size_t capacity,
                          bool date_only) {
  if (capacity != 0 && buffer == nullptr) return {TextStatus::kInvalidInput, 0};
  if (capacity != 0) buffer[0] = '\0';
  bool has_offset;
  if (!ValidFormat(format, format_length, has_offset, date_only))
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
                        size_t format_length, bool date_only,
                        const void* result, Parser& parser) {
  bool has_offset;
  if (!ValidFormat(format, format_length, has_offset, date_only))
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
  if (!parser.isComplete()) return {parser.status, parser.pos};
  return {TextStatus::kOk, parser.pos};
}

}  // namespace

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
      static_cast<int32_t>(value.timeZone().offset().inMinutes())};
  return FormatFields(fields, format, format_length, buffer, capacity, false);
}

FormatResult FormatCivilDay(CivilDay value, const char* format,
                            size_t format_length, char* buffer,
                            size_t capacity) {
  if (!value.isValid()) {
    if (capacity != 0 && buffer != nullptr) buffer[0] = '\0';
    return {TextStatus::kInvalidInput, 0};
  }
  const int32_t fields[kCount] = {value.year(), value.month(), value.day()};
  return FormatFields(fields, format, format_length, buffer, capacity, true);
}

ParseResult ParseDateTime(const char* text, size_t length, const char* format,
                          size_t format_length, UtcOffset default_offset,
                          DateTime* result) {
  Parser parser(text, length);
  const ParseResult parsed =
      ParseFields(text, length, format, format_length, false, result, parser);
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
  const ParseResult parsed =
      ParseFields(text, length, format, format_length, true, result, parser);
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
