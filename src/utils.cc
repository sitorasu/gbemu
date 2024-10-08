#include "utils.h"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace gbemu {

[[noreturn]] void Error(const char* fmt, ...) {
  std::fprintf(stderr, "Error: ");
  std::va_list args;
  va_start(args, fmt);
  std::vfprintf(stderr, fmt, args);
  va_end(args);
  std::fprintf(stderr, "\n");
  std::exit(0);
}

void WarnUser(const char* fmt, ...) {
  std::fprintf(stderr, "Warning: ");
  std::va_list args;
  va_start(args, fmt);
  std::vfprintf(stderr, fmt, args);
  va_end(args);
  std::fprintf(stderr, "\n");
}

std::uint16_t ConcatUInt(std::uint8_t lower, std::uint8_t upper) {
  return lower | (static_cast<std::uint16_t>(upper) << 8);
}

std::uint8_t ExtractBits(std::uint8_t value, unsigned pos, unsigned bits) {
  return (value >> pos) & ((1 << bits) - 1);
}

}  // namespace gbemu
