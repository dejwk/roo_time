#pragma once

#include <cstddef>
#include <cstring>

#include "roo_time/timezone.h"
#include "roo_time/wall_time.h"

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

/// Formats a valid DateTime without allocation. Supported directives are %Y,
/// %m, %d, %H, %M, %S, %f, %z, %:z, %F, %T, and %%; other bytes are literals.
/// Offset directives require an offset from -23:59 through +23:59. Ranges need
/// not be NUL-terminated; null pointers are valid only for empty ranges.
/// Output never overlaps the format and is NUL-terminated when capacity is
/// nonzero. nullptr, 0 queries the required size and returns kBufferTooSmall.
FormatResult FormatDateTime(const DateTime& value, const char* format,
                            size_t format_length, char* buffer,
                            size_t capacity);

/// Strictly parses the entire input without allocation. %f accepts one through
/// six digits, Z denotes UTC, and explicit offsets replace default_offset.
/// Year, month, and day are required; duplicate fields must agree. result is
/// unchanged on failure. No normalization, local timezone, or DST lookup
/// occurs.
ParseResult ParseDateTime(const char* text, size_t length, const char* format,
                          size_t format_length, UtcOffset default_offset,
                          DateTime* result);

/// Formats with a NUL-terminated format string.
inline FormatResult FormatDateTime(const DateTime& value, const char* format,
                                   char* buffer, size_t capacity) {
  return FormatDateTime(value, format,
                        format != nullptr ? std::strlen(format) : 1, buffer,
                        capacity);
}

/// Parses with a NUL-terminated format string.
inline ParseResult ParseDateTime(const char* text, size_t length,
                                 const char* format, UtcOffset default_offset,
                                 DateTime* result) {
  return ParseDateTime(text, length, format,
                       format != nullptr ? std::strlen(format) : 1,
                       default_offset, result);
}

#if ROO_TIME_HAS_STRING_VIEW
/// Formats using a bounded string view format.
inline FormatResult FormatDateTime(const DateTime& value,
                                   roo::string_view format, char* buffer,
                                   size_t capacity) {
  return FormatDateTime(value, format.data(), format.size(), buffer, capacity);
}

/// Parses bounded text and format views.
inline ParseResult ParseDateTime(roo::string_view text, roo::string_view format,
                                 UtcOffset default_offset, DateTime* result) {
  return ParseDateTime(text.data(), text.size(), format.data(), format.size(),
                       default_offset, result);
}
#endif

#if ROO_TIME_HAS_STD_STRING
/// Formats to std::string, returning empty on formatting error or empty output.
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

/// Formats a NUL-terminated format string to std::string.
inline std::string FormatDateTime(const DateTime& value, const char* format) {
  return FormatDateTime(value, format,
                        format != nullptr ? std::strlen(format) : 1);
}

#if ROO_TIME_HAS_STRING_VIEW
/// Formats a bounded string view to std::string.
inline std::string FormatDateTime(const DateTime& value,
                                  roo::string_view format) {
  return FormatDateTime(value, format.data(), format.size());
}
#endif
#endif

#if defined(ARDUINO)
/// Formats to Arduino String, returning empty on formatting or allocation
/// failure.
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

/// Formats a NUL-terminated format string to Arduino String.
inline String FormatDateTimeArduino(const DateTime& value, const char* format) {
  return FormatDateTimeArduino(value, format,
                               format != nullptr ? std::strlen(format) : 1);
}

#if ROO_TIME_HAS_STRING_VIEW
/// Formats a bounded string view to Arduino String.
inline String FormatDateTimeArduino(const DateTime& value,
                                    roo::string_view format) {
  return FormatDateTimeArduino(value, format.data(), format.size());
}
#endif
#endif

/// Canonical ISO 8601 profile: YYYY-MM-DDTHH:MM:SS.ffffff+hh:mm.
constexpr char kIsoDateTimeFormat[] = "%FT%T.%f%:z";
/// Buffer size, including NUL, required by the canonical ISO 8601 profile.
constexpr size_t kIsoDateTimeBufferSize = 33;  // 32 characters plus NUL.

/// Formats using the canonical ISO 8601 profile.
inline FormatResult FormatIsoDateTime(const DateTime& value, char* buffer,
                                      size_t capacity) {
  return FormatDateTime(value, kIsoDateTimeFormat, buffer, capacity);
}

/// Parses the canonical extended ISO 8601 forms with one through six optional
/// fractional digits and a required Z or colon-separated numeric offset.
ParseResult ParseIsoDateTime(const char* text, size_t length, DateTime* result);

/// Parses a NUL-terminated ISO 8601 string.
inline ParseResult ParseIsoDateTime(const char* text, DateTime* result) {
  if (text == nullptr) return {TextStatus::kInvalidInput, 0};
  return ParseIsoDateTime(text, std::strlen(text), result);
}

#if ROO_TIME_HAS_STRING_VIEW
/// Parses a bounded ISO 8601 string view.
inline ParseResult ParseIsoDateTime(roo::string_view text, DateTime* result) {
  return ParseIsoDateTime(text.data(), text.size(), result);
}
#endif

#if ROO_TIME_HAS_STD_STRING
/// Formats to std::string with the canonical ISO 8601 profile.
inline std::string FormatIsoDateTime(const DateTime& value) {
  return FormatDateTime(value, kIsoDateTimeFormat);
}
#endif

#if defined(ARDUINO)
/// Formats to Arduino String with the canonical ISO 8601 profile.
inline String FormatIsoDateTimeArduino(const DateTime& value) {
  return FormatDateTimeArduino(value, kIsoDateTimeFormat);
}

/// Parses an Arduino String as canonical ISO 8601 text.
inline ParseResult ParseIsoDateTime(const String& text, DateTime* result) {
  return ParseIsoDateTime(text.c_str(), text.length(), result);
}
#endif

/// Resolves the zone once, then formats the resulting fixed-offset local time.
/// Resolves the zone once and formats a NUL-terminated format string.
inline FormatResult FormatDateTime(WallTime instant, const TimeZone& zone,
                                   const char* format, size_t format_length,
                                   char* buffer, size_t capacity) {
  const DateTime local = ToLocal(instant, zone);
  return FormatDateTime(local, format, format_length, buffer, capacity);
}

inline FormatResult FormatDateTime(WallTime instant, const TimeZone& zone,
                                   const char* format, char* buffer,
                                   size_t capacity) {
  return FormatDateTime(instant, zone, format,
                        format != nullptr ? std::strlen(format) : 1, buffer,
                        capacity);
}

/// Resolves the zone once and formats canonical ISO 8601 text.
inline FormatResult FormatIsoDateTime(WallTime instant, const TimeZone& zone,
                                      char* buffer, size_t capacity) {
  return FormatDateTime(instant, zone, kIsoDateTimeFormat, buffer, capacity);
}
#if ROO_TIME_HAS_STRING_VIEW
/// Resolves the zone once and formats a bounded format view.
inline FormatResult FormatDateTime(WallTime instant, const TimeZone& zone,
                                   roo::string_view format, char* buffer,
                                   size_t capacity) {
  return FormatDateTime(instant, zone, format.data(), format.size(), buffer,
                        capacity);
}
#endif
#if ROO_TIME_HAS_STD_STRING
/// Resolves the zone once and formats to std::string.
inline std::string FormatDateTime(WallTime instant, const TimeZone& zone,
                                  const char* format, size_t format_length) {
  const DateTime local = ToLocal(instant, zone);
  return FormatDateTime(local, format, format_length);
}

/// Resolves the zone once and formats a NUL-terminated format string to
/// std::string.
inline std::string FormatDateTime(WallTime instant, const TimeZone& zone,
                                  const char* format) {
  return FormatDateTime(instant, zone, format,
                        format != nullptr ? std::strlen(format) : 1);
}

/// Resolves the zone once and formats canonical ISO 8601 text to std::string.
inline std::string FormatIsoDateTime(WallTime instant, const TimeZone& zone) {
  return FormatDateTime(instant, zone, kIsoDateTimeFormat);
}
#if ROO_TIME_HAS_STRING_VIEW
/// Resolves the zone once and formats a bounded format view to std::string.
inline std::string FormatDateTime(WallTime instant, const TimeZone& zone,
                                  roo::string_view format) {
  return FormatDateTime(instant, zone, format.data(), format.size());
}
#endif
#endif
#if defined(ARDUINO)
/// Resolves the zone once and formats to Arduino String.
inline String FormatDateTimeArduino(WallTime instant, const TimeZone& zone,
                                    const char* format, size_t format_length) {
  const DateTime local = ToLocal(instant, zone);
  return FormatDateTimeArduino(local, format, format_length);
}

/// Resolves the zone once and formats a NUL-terminated format string to Arduino
/// String.
inline String FormatDateTimeArduino(WallTime instant, const TimeZone& zone,
                                    const char* format) {
  return FormatDateTimeArduino(instant, zone, format,
                               format != nullptr ? std::strlen(format) : 1);
}

/// Resolves the zone once and formats canonical ISO 8601 text to Arduino
/// String.
inline String FormatIsoDateTimeArduino(WallTime instant, const TimeZone& zone) {
  return FormatDateTimeArduino(instant, zone, kIsoDateTimeFormat);
}
#if ROO_TIME_HAS_STRING_VIEW
/// Resolves the zone once and formats a bounded format view to Arduino String.
inline String FormatDateTimeArduino(WallTime instant, const TimeZone& zone,
                                    roo::string_view format) {
  return FormatDateTimeArduino(instant, zone, format.data(), format.size());
}
#endif
#endif

}  // namespace roo_time
