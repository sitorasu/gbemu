#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "cartridge.h"

using namespace gbemu;
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

namespace {

std::vector<uint8_t> LoadRom(const std::string& path) {
  // ROMファイルをオープン
  std::ifstream ifs(path, std::ios_base::in | std::ios_base::binary);
  if (ifs.fail()) {
    std::cerr << "File open error: " << path << "\n";
    std::exit(0);
  }

  // ROMファイルを読み出す
  std::istreambuf_iterator<char> it_ifs_begin(ifs);
  std::istreambuf_iterator<char> it_ifs_end{};
  std::vector<uint8_t> rom(it_ifs_begin, it_ifs_end);
  if (ifs.fail()) {
    std::cerr << "File read error: " << path << "\n";
    std::exit(0);
  }

  // ROMファイルをクローズ
  ifs.close();
  if (ifs.fail()) {
    std::cerr << "File close error: " << path << "\n";
    std::exit(0);
  }

  return rom;
}

}  // namespace

int main(int argc, char* argv[]) {
  // コマンドラインの書式をチェック
  if (argc < 2) {
    std::cerr << "usage: gbemu <rom_file>\n";
    return 0;
  }

  // ROMファイルをロード
  std::vector<uint8_t> rom(LoadRom(argv[1]));

  Cartridge cartridge(std::move(rom));
  cartridge.mbc().Read8(0);

  return 0;
}
