#include "cpu.h"

#include <cstdint>

#include "memory.h"
#include "utils.h"

namespace gbemu {

std::uint8_t Cpu::step() {
  std::uint16_t pc = registers_.pc();
  std::uint8_t opcode = memory_.Read8(pc);
  if (opcode == 0xCB) {
    // プレフィックスありのオペコードのデコード
    pc++;
    opcode = memory_.Read8(pc);
    switch (opcode) {
      default:
        MyAssert(false, "Unknown opcode: CB %02X", opcode);
    }
  } else {
    // プレフィックスなしのオペコードのデコード
    switch (opcode) {
      default:
        MyAssert(false, "Unknown opcode: %02X", opcode);
    }
  }
  return 0;
}

}  // namespace gbemu
