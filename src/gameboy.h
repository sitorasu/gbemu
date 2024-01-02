#ifndef GBEMU_GAMEBOY_H_
#define GBEMU_GAMEBOY_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "apu.h"
#include "cartridge.h"
#include "cpu.h"
#include "interrupt.h"
#include "joypad.h"
#include "memory.h"
#include "ppu.h"
#include "timer.h"

namespace gbemu {

class GameBoy {
 public:
  GameBoy(Cartridge* cartridge, std::vector<std::uint8_t>* boot_rom = nullptr)
      : cartridge_(cartridge),
        interrupt_(),
        ppu_(interrupt_),
        apu_(),
        timer_(interrupt_),
        joypad_(interrupt_),
        memory_(cartridge_, interrupt_, timer_, joypad_, ppu_, apu_, boot_rom),
        cpu_(memory_, interrupt_) {}

  // 1フレーム進める
  void Step();

  // バッファを取得する
  const GbLcdPixelMatrix& GetBuffer() const { return ppu_.GetBuffer(); }

  // キーを押す。すでに押していたら何も起こらない。
  void PressKey(Joypad::Key key) { joypad_.PressKey(key); }

  // キーを離す。すでに離していたら何も起こらない。
  void ReleaseKey(Joypad::Key key) { joypad_.ReleaseKey(key); }

 private:
  Cartridge* cartridge_;
  Interrupt interrupt_;
  Ppu ppu_;
  Apu apu_;
  Timer timer_;
  Joypad joypad_;
  Memory memory_;
  Cpu cpu_;
};

}  // namespace gbemu

#endif  // GBEMU_GAMEBOY_H_
