#include <SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "command_line.h"
#include "gameboy.h"
#include "renderer.h"
#include "utils.h"

using namespace gbemu;

namespace {

// 指定したパスのファイルをバイナリファイルとして読み出す。
// 読み出しに失敗したらプログラムを終了する。
std::vector<std::uint8_t> LoadBinary(const std::string& path) {
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

void OutputBinary(const std::string& path,
                  const std::vector<std::uint8_t>& data) {
  std::ofstream ofs(path, std::ios_base::out | std::ios_base::binary);
  if (ofs.fail()) {
    Error("File cannot open: %s", path.c_str());
  }
  std::ostreambuf_iterator<char> it_ofs(ofs);
  std::copy(std::cbegin(data), std::cend(data), it_ofs);
  if (ofs.fail()) {
    Error("File cannot read: %s", path.c_str());
  }
  ofs.close();
  if (ofs.fail()) {
    Error("File cannot close: %s", path.c_str());
  }
}

// イベントを処理する。具体的には
// - キー入力をエミュレータに渡す。
// - エミュレータのウインドウの閉じるボタンの押下を検出したら
//   処理を中断してtrueを返す。
bool PollEvent(GameBoy& gb) {
  static std::map<SDL_Keycode, Joypad::Key> keymap{
      {SDLK_w, Joypad::Key::kUp},
      {SDLK_a, Joypad::Key::kLeft},
      {SDLK_s, Joypad::Key::kDown},
      {SDLK_d, Joypad::Key::kRight},
      {SDLK_j, Joypad::Key::kB},
      {SDLK_k, Joypad::Key::kA},
      {SDLK_BACKSPACE, Joypad::Key::kSelect},
      {SDLK_RETURN, Joypad::Key::kStart}};
  SDL_Event e;
  while (SDL_PollEvent(&e) != 0) {
    if (e.type == SDL_QUIT) {
      return true;
    }
    if (e.type == SDL_KEYDOWN) {
      auto sym = e.key.keysym.sym;
      auto i = keymap.find(sym);
      if (i != keymap.end()) {
        gb.PressKey(i->second);
      }
      continue;
    }
    if (e.type == SDL_KEYUP) {
      auto sym = e.key.keysym.sym;
      auto i = keymap.find(sym);
      if (i != keymap.end()) {
        gb.ReleaseKey(i->second);
      }
      continue;
    }
  }
  return false;
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

#define ENABLE_LCD

int main(int argc, char* argv[]) {
  if (!options.Parse(argc, argv)) {
    Error("Usage: gbemu [--debug] [--bootrom <bootrom_file>] --rom <rom_file>");
  }

#ifdef ENABLE_LCD
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    Error("SDL_Init Error: %s", SDL_GetError());
  }

  std::atexit(SDL_Quit);
#endif

  // ROMファイルをロードする
  std::vector<std::uint8_t> rom(LoadBinary(options.rom_file_name()));

  // ブートROMがあるならロードする
  std::vector<std::uint8_t>* boot_rom = nullptr;
  if (options.has_boot_rom()) {
    boot_rom =
        new std::vector<std::uint8_t>(LoadBinary(options.boot_rom_file_name()));
  }

  // セーブファイル（ROMファイルの拡張子をsavに変えたファイル）が
  // ROMファイルと同じ場所にあるならロードする
  std::vector<std::uint8_t> save;
  std::filesystem::path save_file_path = options.rom_file_name();
  save_file_path.replace_extension("sav");
  if (std::filesystem::exists(save_file_path)) {
    save = LoadBinary(save_file_path.c_str());
  }

  Cartridge cartridge(rom, &save);
  GameBoy gb(&cartridge, boot_rom);
#ifndef ENABLE_LCD
  for (;;) {
    gb.Step();
    auto& buffer = gb.GetBuffer();
  }
#else
  {
    Renderer renderer(2);
    if (renderer.vsync()) {
      // 垂直同期オン
      std::cout << "vsync on" << std::endl;
      while (!PollEvent(gb)) {
        gb.Step();
        auto& buffer = gb.GetBuffer();
        renderer.Render(buffer);
      }
    } else {
      // 垂直同期オフ
      std::cout << "vsync off" << std::endl;
      while (!PollEvent(gb)) {
        gb.Step();
        renderer.Render(gb.GetBuffer());

        // 次のフレーム開始時間まで待つ
        WaitForNextFrame();
      }
    }
  }
#endif

  OutputBinary(save_file_path, save);

  return 0;
}
