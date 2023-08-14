#include "utils.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace gbemu {
void MyAssert(bool cond, const char* fmt, ...) {
  if (!cond) {
    fprintf(stderr, "Assertion failed. file: %s, line: %d\n", __FILE__,
            __LINE__);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    std::exit(0);
  }
}
}  // namespace gbemu
