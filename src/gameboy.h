#ifndef GBEMU_GAMEBOY_H_
#define GBEMU_GAMEBOY_H_

#include <memory>

#include "cartridge.h"
#include "cpu.h"
#include "interrupt.h"
#include "memory.h"
#include "ppu.h"
#include "timer.h"

namespace gbemu {

class GameBoy {
 public:
  GameBoy(std::shared_ptr<Cartridge> cartridge)
      : cartridge_(cartridge),
        interrupt_(),
        ppu_(interrupt_),
        timer_(interrupt_),
        memory_(cartridge_, interrupt_, timer_, ppu_),
        cpu_(memory_, interrupt_) {}
  // 1フレーム進める
  void Step();
  const GbLcdPixelMatrix& GetBuffer() const { return ppu_.GetBuffer(); }

 private:
  std::shared_ptr<Cartridge> cartridge_;
  Interrupt interrupt_;
  Ppu ppu_;
  Timer timer_;
  Memory memory_;
  Cpu cpu_;
};

}  // namespace gbemu

#endif  // GBEMU_GAMEBOY_H_
