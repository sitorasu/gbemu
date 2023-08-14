#include "cpu.h"

#include <cstdint>

#include "memory.h"
#include "utils.h"

namespace gbemu {

std::uint8_t Cpu::Nop() {
  std::uint16_t pc = registers_.pc();
  pc += 1;
  registers_.set_pc(pc);
  return 4;
}

std::uint8_t Cpu::Jp() {
  std::uint8_t pc = registers_.pc();
  pc += 1;
  std::uint16_t address = memory_.Read16(pc);
  registers_.set_pc(address);
  return 16;
}

std::uint8_t Cpu::Step() {
  std::uint16_t pc = registers_.pc();
  std::uint8_t opcode = memory_.Read8(pc);
  if (opcode == 0xCB) {
    // プレフィックスありのオペコードのデコード
    pc++;
    opcode = memory_.Read8(pc);
    switch (opcode) {
      default:
        UNREACHABLE("Unknown opcode: CB %02X", opcode);
    }
  } else {
    // プレフィックスなしのオペコードのデコード
    switch (opcode) {
      case 0x00:
        return Nop();
      case 0xC3:
        return Jp();
      default:
        UNREACHABLE("Unknown opcode: %02X", opcode);
    }
  }
}

}  // namespace gbemu
