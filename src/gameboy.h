#ifndef GBEMU_GAMEBOY_H_
#define GBEMU_GAMEBOY_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "cartridge.h"
#include "cpu.h"
#include "interrupt.h"
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
        timer_(interrupt_),
        memory_(cartridge_, interrupt_, timer_, ppu_, boot_rom),
        cpu_(memory_, interrupt_) {}

  // 1フレーム進める
  void Step();
  const GbLcdPixelMatrix& GetBuffer() const { return ppu_.GetBuffer(); }

 private:
  Cartridge* cartridge_;
  Interrupt interrupt_;
  Ppu ppu_;
  Timer timer_;
  Memory memory_;
  Cpu cpu_;
};

}  // namespace gbemu

#endif  // GBEMU_GAMEBOY_H_
