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

std::uint16_t ConcatUInt(std::uint8_t lower, std::uint8_t upper) {
  return lower | (static_cast<std::uint16_t>(upper) << 8);
}

}  // namespace gbemu
