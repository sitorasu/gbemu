#include "cpu.h"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

#include "command_line.h"
#include "memory.h"
#include "utils.h"

namespace gbemu {

namespace {
class Nop : public Instruction {
 public:
  Nop(std::uint16_t address) : Instruction(0, address) {}
  std::string GetMnemonicString() override { return "nop"; }
  unsigned length() override { return 1; }
  unsigned cycles() override { return 4; }
};

class JpU16 : public Instruction {
 public:
  JpU16(std::uint32_t raw_code, std::uint16_t address, std::uint16_t imm)
      : Instruction(raw_code, address), imm_(imm) {}
  std::string GetMnemonicString() override {
    char buf[16];
    std::sprintf(buf, "jp %04X", imm_);
    return std::string(buf);
  }
  unsigned length() override { return 3; }
  unsigned cycles() override { return 16; }

 private:
  std::uint16_t imm_;
};

}  // namespace

std::shared_ptr<Instruction> Cpu::ExecuteNop() {
  std::uint16_t pc = registers_.pc();
  std::shared_ptr<Instruction> nop(new Nop(pc));
  registers_.set_pc(pc + nop->length());
  return nop;
}

std::shared_ptr<Instruction> Cpu::ExecuteJpU16() {
  std::uint16_t pc = registers_.pc();
  std::uint8_t opcode = memory_.Read8(pc);
  std::uint16_t imm = memory_.Read16(pc + 1);
  std::uint32_t raw_code = ((std::uint32_t)imm << 8) | opcode;
  registers_.set_pc(imm);
  return std::shared_ptr<Instruction>(new JpU16(raw_code, pc, imm));
}

std::shared_ptr<Instruction> Cpu::ExecutePrefixedInstruction() {
  std::uint16_t pc = registers_.pc();
  std::uint8_t opcode = memory_.Read8(pc + 1);
  UNREACHABLE("Unknown opcode: CB %02X", opcode);
}

std::shared_ptr<Instruction> Cpu::ExecuteInstruction() {
  std::uint16_t pc = registers_.pc();
  std::uint8_t opcode = memory_.Read8(pc);
  if (opcode == 0xCB) {
    // プレフィックスありの命令の実行
    return ExecutePrefixedInstruction();
  }
  if (opcode == 0x00) {
    return ExecuteNop();
  }
  if (opcode == 0xC3) {
    return ExecuteJpU16();
  }
  UNREACHABLE("Unknown opcode: %02X\n", opcode);
}

namespace {
// 0xABCDEF -> "EFCDAB"
std::string ConvertToLittleEndianString(std::uint32_t value) {
  char buf[16];
  char* p = buf;
  do {
    sprintf(p, "%02X", value & 0xFF);
    value >>= 8;
    p += 2;
  } while (value != 0);
  return std::string(buf);
}

void PrintInstructionInfo(std::shared_ptr<Instruction> inst) {
  char buf[64];
  std::string raw_code = ConvertToLittleEndianString(inst->raw_code());
  std::string mnemonic = inst->GetMnemonicString();
  std::sprintf(buf, "$%04X %s\t%s", inst->address(), raw_code.c_str(),
               mnemonic.c_str());
  std::printf("%s\n", buf);
}
}  // namespace

unsigned Cpu::Step() {
  std::shared_ptr<Instruction> inst = ExecuteInstruction();
  if (options.debug()) {
    PrintInstructionInfo(inst);
  }
  return inst->cycles();
}

}  // namespace gbemu
