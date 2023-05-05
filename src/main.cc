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
  std::vector<char> input_data(it_ifs_begin, it_ifs_end);
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

  if (input_data.size() < 150) {
    std::cerr << "ROM format error\n";
    return 0;
  }

  CartridgeHeader header =
      CartridgeHeader::create(reinterpret_cast<uint8_t*>(input_data.data()));
  header.print();

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
