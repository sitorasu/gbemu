#ifndef GBEMU_COMMAND_LINE_H_
#define GBEMU_COMMAND_LINE_H_

#include <string>

namespace gbemu {

class Options {
 public:
  // オプションをパースする。成功したらtrueを、失敗したらfalseを返す。
  bool Parse(int argc, char* argv[]);

  bool debug() { return debug_; }
  bool has_boot_rom() { return has_boot_rom_; }
  std::string boot_rom_file_name() { return boot_rom_file_name_; }
  std::string rom_file_name() { return rom_file_name_; }

 private:
  bool debug_;
  bool has_boot_rom_;
  std::string boot_rom_file_name_;
  std::string rom_file_name_;
};

extern Options options;

}  // namespace gbemu

#endif  // GBEMU_COMMAND_LINE_H_
