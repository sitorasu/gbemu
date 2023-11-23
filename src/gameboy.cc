#include "gameboy.h"

namespace gbemu {

void GameBoy::Run() {
  for (;;) {
    unsigned mcycle = cpu_.Step();
    timer_.step(mcycle * 4);
  }
}

}
