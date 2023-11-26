#include <SDL.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "command_line.h"
#include "gameboy.h"
#include "renderer.h"
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

bool PollQuit() {
  SDL_Event e;
  while (SDL_PollEvent(&e) != 0) {
    if (e.type == SDL_QUIT) {
      return true;
    }
  }
  return false;
}

void UpdateLCDPixels(LCDPixels& pixels) {
  static int j;
  static GBColor color = kLightGray;
  for (int i = 0; i < kGBHorizontalPixels; i++) {
    pixels[i][j] = color;
  }

  j = (j + 1) % kGBVerticalPixels;
  if (j == 0) {
    color = static_cast<GBColor>((color + 1) % kGBColorNum);
  }
}

void WaitForNextFrame() {
  static int frame_count = 0;
  constexpr int kFramesInSec = 60;
  static Uint64 current_sec_start = SDL_GetTicks64();
  if (frame_count == kFramesInSec) {
    frame_count = 0;
    current_sec_start = SDL_GetTicks64();
  }

  frame_count++;
  Uint64 next_frame_start =
      current_sec_start + 1000 * frame_count / kFramesInSec;
  Uint64 now = SDL_GetTicks64();
  if (now < next_frame_start) {
    SDL_Delay(next_frame_start - now);
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  options.Parse(argc, argv);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    Error("SDL_Init Error: %s", SDL_GetError());
  }

  std::atexit(SDL_Quit);

  {
    Renderer renderer(1);
    LCDPixels pixels = {};
    while (!PollQuit()) {
      // ゲームボーイを1フレーム動作させる
      // 描画すべきグラフィックを取得する
      // 描画する
      UpdateLCDPixels(pixels);
      renderer.RenderLCDPixels(pixels);

      // 次のフレーム開始時間まで待つ
      WaitForNextFrame();
    }
  }

#if 0
  // ROMファイルをロード
  std::vector<std::uint8_t> rom(LoadRom(options.filename()));

  std::shared_ptr<Cartridge> cartridge =
      std::make_shared<Cartridge>(std::move(rom));
  GameBoy gb(cartridge);
  gb.Run();
#endif

  return 0;
}
