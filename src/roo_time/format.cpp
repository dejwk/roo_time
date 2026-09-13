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

  bool next(char& code, char& literal) {
    char c;
    if (!get(c)) return false;
    code = '\0';
    literal = c;
    if (c == '\0') {
      valid_ = false;
    } else if (c == '%') {
      if (!get(code)) {
        valid_ = false;
      } else if (code == 'F' || code == 'T') {
        alias_ = code == 'F' ? "%Y-%m-%d" : "%H:%M:%S";
        return next(code, literal);
      } else if (code == '%') {
        code = '\0';
        literal = '%';
      } else if (code == ':') {
        char z;
        if (!get(z) || z != 'z') valid_ = false;
      } else if (code == '\0' || std::strchr("YmdHMSfz", code) == nullptr) {
        valid_ = false;
      }
    }
    return valid_;
  }

  bool valid() const { return valid_; }

 private:
  bool get(char& c) {
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

bool ValidFormat(const char* format, size_t length, bool& has_offset) {
  if (format == nullptr && length != 0) return false;
  Tokens tokens(format, length);
  char code, literal;
  has_offset = false;
  while (tokens.next(code, literal)) {
    if (code == 'z' || code == ':') has_offset = true;
  }
  return tokens.valid();
}

class Writer {
 public:
  Writer(char* buffer, size_t capacity)
      : buffer_(buffer), capacity_(capacity) {}
  void put(char c) {
    // Saturate rather than wrap on targets with a small size_t. SIZE_MAX bytes
    // cannot be represented together with the required terminating NUL.
    if (size_ == static_cast<size_t>(-1)) return;
    if (capacity_ != 0 && size_ < capacity_ - 1) buffer_[size_] = c;
    ++size_;
  }
  void number(unsigned value, unsigned width) {
    char digits[6];
    for (unsigned i = width; i > 0; --i) {
      digits[i - 1] = '0' + value % 10;
      value /= 10;
    }
    for (unsigned i = 0; i < width; ++i) put(digits[i]);
  }
  FormatResult finish() {
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

void CalendarNumber(Writer& writer, const DateTime& value, char code,
                    unsigned number) {
#if ROO_TIME_HAS_STRFTIME
  std::tm calendar = {};
  calendar.tm_year = value.year() - 1900;
  calendar.tm_mon = value.month() - 1;
  calendar.tm_mday = value.day();
  calendar.tm_hour = value.hour();
  calendar.tm_min = value.minute();
  calendar.tm_sec = value.second();
  calendar.tm_isdst = -1;
  const char format[] = {'%', code, '\0'};
  char text[3];
  // Only fixed-width numeric fields are delegated. Check the result so locale
  // or libc quirks cannot change the portable output. Years, fractions and
  // offsets always use the portable path.
  if (std::strftime(text, sizeof(text), format, &calendar) == 2 &&
      IsDigit(text[0]) && IsDigit(text[1]) &&
      static_cast<unsigned>((text[0] - '0') * 10 + text[1] - '0') == number) {
    writer.put(text[0]);
    writer.put(text[1]);
    return;
  }
#else
  (void)value;
  (void)code;
#endif
  writer.number(number, 2);
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

class Parser {
 public:
  Parser(const char* text, size_t length) : text_(text), length_(length) {}

  bool literal(char c) {
    if (pos == length_ || text_[pos] != c)
      return fail(TextStatus::kInvalidInput);
    ++pos;
    return true;
  }

  bool number(unsigned width, int& value) {
    value = 0;
    for (unsigned i = 0; i < width; ++i) {
      if (!digit()) return fail(TextStatus::kInvalidInput);
      value = value * 10 + text_[pos++] - '0';
    }
    return true;
  }

  bool directive(char code) {
    const size_t start = pos;
    int value = 0;
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
        while (digits < 6 && digit()) {
          value = value * 10 + text_[pos++] - '0';
          ++digits;
        }
        if (digits == 0) return fail(TextStatus::kInvalidInput);
        if (digit()) return fail(TextStatus::kOutOfRange);
        while (digits++ < 6) value *= 10;
        return set(kMicros, value, start);
      }
      default: {
        // Both offset directives accept the UTC designator.
        if (pos < length_ && text_[pos] == 'Z') {
          ++pos;
          return set(kOffset, 0, start);
        }
        if (pos == length_ || (text_[pos] != '+' && text_[pos] != '-')) {
          return fail(TextStatus::kInvalidInput);
        }
        const bool negative = text_[pos++] == '-';
        int hours, minutes;
        if (!number(2, hours)) return false;
        if (code == ':' && !literal(':')) return false;
        if (!number(2, minutes)) return false;
        if (hours > 23 || minutes > 59)
          return fail(TextStatus::kOutOfRange, start);
        value = hours * 60 + minutes;
        return set(kOffset, negative ? -value : value, start);
      }
    }
    if (!number(width, value)) return false;
    if (value < minimum || value > maximum)
      return fail(TextStatus::kOutOfRange, start);
    return set(field, value, start);
  }

  bool complete() {
    if (pos != length_ || !seen[kYear] || !seen[kMonth] || !seen[kDay]) {
      return fail(TextStatus::kInvalidInput);
    }
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const int year = fields[kYear], month = fields[kMonth];
    int max_day = days[month - 1];
    if (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
      ++max_day;
    if (fields[kDay] > max_day)
      return fail(TextStatus::kOutOfRange, positions[kDay]);
    return true;
  }

  size_t pos = 0;
  TextStatus status = TextStatus::kOk;
  int fields[kCount] = {};
  bool seen[kCount] = {};

 private:
  bool digit() const { return pos < length_ && IsDigit(text_[pos]); }
  bool fail(TextStatus error) {
    status = error;
    return false;
  }
  bool fail(TextStatus error, size_t position) {
    pos = position;
    return fail(error);
  }
  bool set(Field field, int value, size_t start) {
    if (seen[field] && fields[field] != value)
      return fail(TextStatus::kInvalidInput, start);
    fields[field] = value;
    seen[field] = true;
    positions[field] = start;
    return true;
  }
  const char* text_;
  size_t length_;
  size_t positions[kCount] = {};
};

}  // namespace

FormatResult FormatDateTime(const DateTime& value, const char* format,
                            size_t format_length, char* buffer,
                            size_t capacity) {
  if (capacity != 0 && buffer == nullptr) return {TextStatus::kInvalidInput, 0};
  if (capacity != 0) buffer[0] = '\0';
  bool has_offset;
  if (!ValidFormat(format, format_length, has_offset))
    return {TextStatus::kInvalidFormat, 0};
  int offset = static_cast<int>(value.timeZone().offset().inMinutes());
  if (has_offset && (offset < -1439 || offset > 1439))
    return {TextStatus::kOutOfRange, 0};

  Writer writer(buffer, capacity);
  Tokens tokens(format, format_length);
  char code, literal;
  while (tokens.next(code, literal)) {
    switch (code) {
      case '\0':
        writer.put(literal);
        break;
      case 'Y':
        writer.number(value.year(), 4);
        break;
      case 'm':
        CalendarNumber(writer, value, code, value.month());
        break;
      case 'd':
        CalendarNumber(writer, value, code, value.day());
        break;
      case 'H':
        CalendarNumber(writer, value, code, value.hour());
        break;
      case 'M':
        CalendarNumber(writer, value, code, value.minute());
        break;
      case 'S':
        CalendarNumber(writer, value, code, value.second());
        break;
      case 'f':
        writer.number(value.micros(), 6);
        break;
      default: {
        writer.put(offset < 0 ? '-' : '+');
        const unsigned magnitude = offset < 0 ? -offset : offset;
        writer.number(magnitude / 60, 2);
        if (code == ':') writer.put(':');
        writer.number(magnitude % 60, 2);
        break;
      }
    }
  }
  return writer.finish();
}

ParseResult ParseDateTime(const char* text, size_t length, const char* format,
                          size_t format_length, UtcOffset default_offset,
                          DateTime* result) {
  bool has_offset;
  if (!ValidFormat(format, format_length, has_offset))
    return {TextStatus::kInvalidFormat, 0};
  if ((text == nullptr && length != 0) || result == nullptr)
    return {TextStatus::kInvalidInput, 0};
  Parser parser(text, length);
  Tokens tokens(format, format_length);
  char code, literal;
  while (tokens.next(code, literal)) {
    if (!(code != '\0' ? parser.directive(code) : parser.literal(literal))) {
      return {parser.status, parser.pos};
    }
  }
  if (!parser.complete()) return {parser.status, parser.pos};
  const int* f = parser.fields;
  *result = DateTime(
      f[kYear], f[kMonth], f[kDay], f[kHour], f[kMinute], f[kSecond],
      f[kMicros],
      parser.seen[kOffset] ? UtcOffset(Minutes(f[kOffset])) : default_offset);
  return {TextStatus::kOk, parser.pos};
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
