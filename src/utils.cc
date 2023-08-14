#include "utils.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace gbemu {

[[noreturn]] void Error(const char* fmt, ...) {
  fprintf(stderr, "Error: ");
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  std::exit(0);
}

}  // namespace gbemu
