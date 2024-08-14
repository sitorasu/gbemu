#include "gameboy.h"

namespace gbemu {

void GameBoy::Step() {
  while (!ppu_.IsBufferReady()) {
    unsigned mcycles = cpu_.Step();
    unsigned tcycles = mcycles * 4;
    memory_.RunDma(mcycles);
    timer_.Run(tcycles);
    apu_.Run(tcycles);
    ppu_.Run(tcycles);
  }
  ppu_.ResetBufferReadyFlag();
}

}  // namespace gbemu
