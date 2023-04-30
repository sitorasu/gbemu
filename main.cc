#include <cstdlib>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
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

  // 32KiBの読み出し
  constexpr auto ROM_SIZE = 32 * 1024;
  char rom[ROM_SIZE]{};
  ifs.read(rom, sizeof(rom));

  // 動作確認：読み出したデータをそのまま書き出す
  constexpr auto out_path = "test.bin";
  std::ofstream ofs(out_path, std::ios_base::out | std::ios_base::binary);
  if (ofs.fail()) {
    std::cerr << "File open error: " << out_path << "\n";
    return 0;
  }
  ofs.write(rom, sizeof(rom));

  // ROMファイルをクローズ
  ifs.close();
  if (ifs.fail()) {
    std::cerr << "File close error: " << path << "\n";
    return 0;
  }

  return 0;
}
