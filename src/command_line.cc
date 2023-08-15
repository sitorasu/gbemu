#include "command_line.h"

#include <string>

#include "utils.h"

namespace gbemu {

Options options;

void Options::Parse(int argc, char* argv[]) {
  if (argc < 2) {
    Error("Usage: gbemu [--debug] <rom_file>");
  }

  int i = 1;
  std::string cur = argv[i];
  if (cur == "--debug") {
    debug_ = true;
    i++;
    if (i == argc) {
      Error("Usage: gbemu [--debug] <rom_file>");
    }
  }

  filename_ = argv[i++];
}

}  // namespace gbemu
