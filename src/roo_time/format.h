#pragma once

#include <cstddef>
#include <cstring>

#include "roo_time.h"

// Header detection is conservative on older toolchains. Define to 1 only when
// <string> and std::string are usable, or to 0 to omit allocating overloads.
#ifndef ROO_TIME_HAS_STD_STRING
#if defined(__has_include)
#if __has_include(<string>)
#define ROO_TIME_HAS_STD_STRING 1
#endif
#endif
#endif
#ifndef ROO_TIME_HAS_STD_STRING
#define ROO_TIME_HAS_STD_STRING 0
#endif

// roo_backport/string_view.h itself requires <string>. Override independently
// to keep view overloads when disabling allocating overloads on a capable STL.
#ifndef ROO_TIME_HAS_STRING_VIEW
#define ROO_TIME_HAS_STRING_VIEW ROO_TIME_HAS_STD_STRING
#endif

#if ROO_TIME_HAS_STD_STRING
#include <string>
#endif
#if ROO_TIME_HAS_STRING_VIEW
#include "roo_backport/string_view.h"
#endif
#if defined(ARDUINO)
#include <WString.h>

#include <climits>
#include <cstdlib>
#endif

namespace roo_time {

enum class TextStatus {
  kOk,
  kInvalidFormat,
  kInvalidInput,
  kOutOfRange,
  kBufferTooSmall,
};

struct FormatResult {
  TextStatus status;
  // Required bytes excluding NUL for kOk/kBufferTooSmall; zero otherwise.
  size_t size;
};

struct ParseResult {
  TextStatus status;
  // Consumed input bytes on success; input error offset on failure. Invalid
  // formats report zero. Missing fields report the end of the input.
  size_t position;
};

// Formats a valid DateTime, without allocation. Supported directives:
// %Y (4 digits), %m/%d/%H/%M/%S (2 digits), %f (6 digits), %z (+hhmm),
// %:z (+hh:mm), %F (%Y-%m-%d), %T (%H:%M:%S), and %%. Other bytes are
// matched literally, except embedded NULs, which are invalid in formats.
// Offset directives require an offset between -23:59 and +23:59 inclusive.
// Locale and the process timezone do not affect the result.
//
// Input ranges need not be NUL-terminated. Null pointers are allowed only for
// empty ranges. Output must not overlap the format. When capacity > 0, buffer
// must be nonnull and is always NUL-terminated. Insufficient space yields a
// truncated prefix; other errors yield an empty buffer. nullptr, 0 queries size
// (kBufferTooSmall, including for an empty format, whose NUL needs one byte).
FormatResult FormatDateTime(const DateTime& value, const char* format,
                            size_t format_length, char* buffer,
                            size_t capacity);

// Strictly parses the entire input without allocation. Numeric widths match
// formatting, except %f accepts 1-6 digits, scaled to microseconds. Offset
// directives also accept Z for UTC; an explicit offset replaces default_offset.
// Year, month, and day are required; absent time fields default to zero.
// Duplicate fields must agree. Invalid dates, leap seconds, and out-of-range
// values are rejected before construction. result must be nonnull and remains
// unchanged on failure. No normalization, local timezone, or DST lookup occurs.
ParseResult ParseDateTime(const char* text, size_t length, const char* format,
                          size_t format_length, UtcOffset default_offset,
                          DateTime* result);

// NUL-terminated format conveniences. The input text remains length-bounded.
inline FormatResult FormatDateTime(const DateTime& value, const char* format,
                                   char* buffer, size_t capacity) {
  return FormatDateTime(value, format,
                        format != nullptr ? std::strlen(format) : 1, buffer,
                        capacity);
}

inline ParseResult ParseDateTime(const char* text, size_t length,
                                 const char* format, UtcOffset default_offset,
                                 DateTime* result) {
  return ParseDateTime(text, length, format,
                       format != nullptr ? std::strlen(format) : 1,
                       default_offset, result);
}

#if ROO_TIME_HAS_STRING_VIEW
inline FormatResult FormatDateTime(const DateTime& value,
                                   roo::string_view format, char* buffer,
                                   size_t capacity) {
  return FormatDateTime(value, format.data(), format.size(), buffer, capacity);
}

inline ParseResult ParseDateTime(roo::string_view text, roo::string_view format,
                                 UtcOffset default_offset, DateTime* result) {
  return ParseDateTime(text.data(), text.size(), format.data(), format.size(),
                       default_offset, result);
}
#endif

#if ROO_TIME_HAS_STD_STRING
// Returns an empty string on formatting error (also the successful result of
// an empty format). Allocation failures follow normal std::string behavior.
inline std::string FormatDateTime(const DateTime& value, const char* format,
                                  size_t format_length) {
  FormatResult measured =
      FormatDateTime(value, format, format_length, nullptr, 0);
  if (measured.status != TextStatus::kBufferTooSmall || measured.size == 0) {
    return {};
  }
  // Include writable space for NUL, also on pre-C++17 standard libraries.
  std::string text(measured.size + 1, '\0');
  FormatResult written =
      FormatDateTime(value, format, format_length, &text[0], text.size());
  if (written.status != TextStatus::kOk) return {};
  text.resize(written.size);
  return text;
}

inline std::string FormatDateTime(const DateTime& value, const char* format) {
  return FormatDateTime(value, format,
                        format != nullptr ? std::strlen(format) : 1);
}

#if ROO_TIME_HAS_STRING_VIEW
inline std::string FormatDateTime(const DateTime& value,
                                  roo::string_view format) {
  return FormatDateTime(value, format.data(), format.size());
}
#endif
#endif

#if defined(ARDUINO)
// Arduino convenience, independent of std::string support. Returns an empty
// String on formatting or allocation failure, or for a successful empty format.
inline String FormatDateTimeArduino(const DateTime& value, const char* format,
                                    size_t format_length) {
  char local[64];
  FormatResult formatted =
      FormatDateTime(value, format, format_length, local, sizeof(local));
  if (formatted.status == TextStatus::kOk) return String(local);
  if (formatted.status != TextStatus::kBufferTooSmall ||
      formatted.size >= UINT_MAX)
    return String();
  char* buffer = static_cast<char*>(std::malloc(formatted.size + 1));
  if (buffer == nullptr) return String();
  FormatResult written =
      FormatDateTime(value, format, format_length, buffer, formatted.size + 1);
  String text = written.status == TextStatus::kOk ? String(buffer) : String();
  std::free(buffer);
  return text;
}

inline String FormatDateTimeArduino(const DateTime& value, const char* format) {
  return FormatDateTimeArduino(value, format,
                               format != nullptr ? std::strlen(format) : 1);
}

#if ROO_TIME_HAS_STRING_VIEW
inline String FormatDateTimeArduino(const DateTime& value,
                                    roo::string_view format) {
  return FormatDateTimeArduino(value, format.data(), format.size());
}
#endif
#endif

// The library's canonical ISO 8601 extended representation preserves the fixed
// offset and microseconds: YYYY-MM-DDTHH:MM:SS.ffffff+hh:mm. UTC uses +00:00.
// This is a specific profile, not support for every ISO 8601 representation.
constexpr char kIsoDateTimeFormat[] = "%FT%T.%f%:z";
constexpr size_t kIsoDateTimeBufferSize = 33;  // 32 characters plus NUL.

// Same status, buffer, and offset-range contracts as FormatDateTime.
inline FormatResult FormatIsoDateTime(const DateTime& value, char* buffer,
                                      size_t capacity) {
  return FormatDateTime(value, kIsoDateTimeFormat, buffer, capacity);
}

// Accepts YYYY-MM-DDTHH:MM:SS[.fraction](Z|+hh:mm|-hh:mm), with 1-6
// fractional digits when present. T and Z must be uppercase; an explicit offset
// is required. No whitespace, basic dates, week dates, or offset without a
// colon. Uses ParseDateTime's strict validation and leaves result unchanged on
// failure.
ParseResult ParseIsoDateTime(const char* text, size_t length, DateTime* result);

inline ParseResult ParseIsoDateTime(const char* text, DateTime* result) {
  if (text == nullptr) return {TextStatus::kInvalidInput, 0};
  return ParseIsoDateTime(text, std::strlen(text), result);
}

#if ROO_TIME_HAS_STRING_VIEW
inline ParseResult ParseIsoDateTime(roo::string_view text, DateTime* result) {
  return ParseIsoDateTime(text.data(), text.size(), result);
}
#endif

#if ROO_TIME_HAS_STD_STRING
// Empty on formatting error; allocation follows normal std::string behavior.
inline std::string FormatIsoDateTime(const DateTime& value) {
  return FormatDateTime(value, kIsoDateTimeFormat);
}
#endif

#if defined(ARDUINO)
// Empty on formatting/allocation failure, independent of std::string support.
inline String FormatIsoDateTimeArduino(const DateTime& value) {
  return FormatDateTimeArduino(value, kIsoDateTimeFormat);
}

inline ParseResult ParseIsoDateTime(const String& text, DateTime* result) {
  return ParseIsoDateTime(text.c_str(), text.length(), result);
}
#endif

}  // namespace roo_time
