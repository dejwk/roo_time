#pragma once

#include <cstdlib>
#include <cstring>

// Minimal owning Arduino String stand-in. Deliberately independent of <string>.
// Only the constructor, copy constructor, and c_str() are needed by the
// adapter.
class String {
 public:
  static bool fail_allocation;

  String(const char* text = "") : data_(nullptr) {
    if (fail_allocation) return;
    const size_t size = std::strlen(text) + 1;
    data_ = static_cast<char*>(std::malloc(size));
    if (data_ != nullptr) std::memcpy(data_, text, size);
  }
  String(const String& other) : String(other.c_str()) {}
  ~String() { std::free(data_); }
  String& operator=(const String&) = delete;
  const char* c_str() const { return data_ != nullptr ? data_ : ""; }
  unsigned int length() const { return std::strlen(c_str()); }

 private:
  char* data_;
};
