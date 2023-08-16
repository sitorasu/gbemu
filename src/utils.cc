#include "utils.h"

#include <cstdarg>
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

}  // namespace gbemu
