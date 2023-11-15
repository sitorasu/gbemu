#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "cartridge.h"
#include "command_line.h"
#include "cpu.h"
#include "interrupt.h"
#include "memory.h"
#include "utils.h"

using namespace gbemu;

namespace {

// `path`のファイルを読み出す。
// 読み出しに失敗したらプログラムを終了する。
std::vector<std::uint8_t> LoadRom(const std::string& path) {
  // ROMファイルをオープン
  std::ifstream ifs(path, std::ios_base::in | std::ios_base::binary);
  if (ifs.fail()) {
    Error("File cannot open: %s", path.c_str());
  }

  // ROMファイルを読み出す
  std::istreambuf_iterator<char> it_ifs_begin(ifs);
  std::istreambuf_iterator<char> it_ifs_end{};
  std::vector<std::uint8_t> rom(it_ifs_begin, it_ifs_end);
  if (ifs.fail()) {
    Error("File cannot read: %s", path.c_str());
  }

  // ROMファイルをクローズ
  ifs.close();
  if (ifs.fail()) {
    Error("File cannot close: %s", path.c_str());
  }

  return rom;
}

}  // namespace

int main(int argc, char* argv[]) {
  options.Parse(argc, argv);

  // ROMファイルをロード
  std::vector<std::uint8_t> rom(LoadRom(options.filename()));

  Cartridge cartridge(std::move(rom));
  Interrupt interrupt;
  Memory memory(cartridge, interrupt);
  Cpu cpu(memory, interrupt);
  for (;;) {
    cpu.Step();
  }

  return 0;
}
