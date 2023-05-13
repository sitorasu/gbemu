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
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

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

  CartridgeHeader::Create(input_data);

  return 0;
}
