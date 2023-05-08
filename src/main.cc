#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "cartridge_header.h"

using namespace gbemu;
using uint8_t = std::uint8_t;
using uint16_t = std::uint16_t;
using uint32_t = std::uint32_t;

int main(int argc, char* argv[]) {
  // コマンドラインの書式をチェック
  if (argc < 2) {
    std::cerr << "usage: gbemu ROM_FILE\n";
    return 0;
  }

  // ROMファイルをオープン
  std::string path = argv[1];
  std::ifstream ifs(path, std::ios_base::in | std::ios_base::binary);
  if (ifs.fail()) {
    std::cerr << "File open error: " << path << "\n";
    return 0;
  }

  // ROMファイルを読み出す
  std::istreambuf_iterator<char> it_ifs_begin(ifs);
  std::istreambuf_iterator<char> it_ifs_end{};
  std::vector<uint8_t> input_data(it_ifs_begin, it_ifs_end);
  if (ifs.fail()) {
    std::cerr << "File read error: " << path << "\n";
    return 0;
  }

  // ROMファイルをクローズ
  ifs.close();
  if (ifs.fail()) {
    std::cerr << "File close error: " << path << "\n";
    return 0;
  }

  // Cartridge headerの分のサイズすらないならエラー
  if (input_data.size() < 0x150) {
    std::cerr << "ROM format error\n";
    return 0;
  }

  // Cartridge headerの読み出し
  CartridgeHeader header(input_data);
  header.Print();

  // Cartridge headerに書かれているサイズと実際のサイズが異なるならエラー
  if (header.rom_size * 1024 != input_data.size()) {
    std::cerr << "ROM size error:\n"
              << "  header's' rom size = " << header.rom_size * 1024 << "\n"
              << "  actual rom size = " << input_data.size() << "\n";
    return 0;
  }

  switch (header.type) {
    case CartridgeHeader::CartridgeType::kRomOnly:
      break;
    case CartridgeHeader::CartridgeType::kMbc1:
      break;
    default:
      std::cerr << "Not implemented cartridge type.\n";
      return 0;
  }

  return 0;
}
