#include "command_line.h"

#include <string>

#include "utils.h"

namespace gbemu {

Options options;

bool Options::Parse(int argc, char* argv[]) {
  if (argc < 2) {
    return false;
  }

  int i = 1;
  while (i < argc) {
    std::string str = argv[i];
    if (str == "--debug") {
      debug_ = true;
      i++;
    } else if (str == "--bootrom") {
      i++;
      if (i == argc) {
        return false;
      }
      boot_rom_file_name_ = argv[i];
      has_boot_rom_ = true;
      i++;
    } else if (str == "--rom") {
      i++;
      if (i == argc) {
        return false;
      }
      rom_file_name_ = argv[i];
      i++;
    } else {
      return false;
    }
  }
  return true;
}

}  // namespace gbemu
