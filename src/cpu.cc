#include "cpu.h"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>

#include "command_line.h"
#include "memory.h"
#include "utils.h"

namespace gbemu {

std::string Cpu::Nop::GetMnemonicString() { return "nop"; }

void Cpu::Nop::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers_.pc();
  cpu.registers_.set_pc(pc + length());
}

std::string Cpu::JpU16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "jp 0x%04X", imm_);
  return std::string(buf);
}

void Cpu::JpU16::Execute(Cpu& cpu) { cpu.registers_.set_pc(imm_); }

std::shared_ptr<Cpu::Instruction> Cpu::FetchPrefixedInstruction() {
  std::uint16_t pc = registers_.pc();
  std::uint8_t opcode = memory_.Read8(pc + 1);
  UNREACHABLE("Unknown opcode: CB %02X", opcode);
}

std::shared_ptr<Cpu::Instruction> Cpu::FetchNop() {
  std::uint16_t pc = registers_.pc();
  return std::shared_ptr<Instruction>(new Nop(pc));
}

std::shared_ptr<Cpu::Instruction> Cpu::FetchJpU16() {
  std::uint16_t pc = registers_.pc();
  std::uint8_t opcode = memory_.Read8(pc);
  std::uint16_t imm = memory_.Read16(pc + 1);
  std::vector<std::uint8_t> raw_code{opcode,
                                     static_cast<std::uint8_t>(imm & 0xFF),
                                     static_cast<std::uint8_t>(imm >> 8)};
  return std::shared_ptr<Instruction>(new JpU16(std::move(raw_code), pc, imm));
}

std::shared_ptr<Cpu::Instruction> Cpu::FetchInstruction() {
  std::uint16_t pc = registers_.pc();
  std::uint8_t opcode = memory_.Read8(pc);
  if (opcode == 0xCB) {
    // プレフィックスありの命令の実行
    return FetchPrefixedInstruction();
  }
  if (opcode == 0x00) {
    return FetchNop();
  }
  if (opcode == 0xC3) {
    return FetchJpU16();
  }
  UNREACHABLE("Unknown opcode: %02X\n", opcode);
}

namespace {
// バイト列を文字列に変換する。
// 例：{ 0xAB, 0xCD, 0xEF } -> "AB CD EF"
std::string Join(const std::vector<std::uint8_t>& v) {
  ASSERT(v.size() <= 4, "raw_code size is too large.");
  char buf[16];
  char* p = buf;
  for (std::uint8_t num : v) {
    sprintf(p, "%02X ", num);
    p += 3;
  }
  std::string str{buf};
  str.pop_back();
  return str;
}
}  // namespace

unsigned Cpu::Step() {
  std::shared_ptr<Instruction> inst = FetchInstruction();
  Execute(inst);

  // デバッグモードなら命令の情報を表示
  // 表示例
  // $0100 00         nop
  // $0101 C3 37 06   jp 0x0637
  // $0637 C3 30 04   jp 0x0430
  // ...
  if (options.debug()) {
    char buf[64];
    std::string raw_code = Join(inst->raw_code());
    std::string mnemonic = inst->GetMnemonicString();
    std::sprintf(buf, "$%04X %s\t%s", inst->address(), raw_code.c_str(),
                 mnemonic.c_str());
    std::printf("%s\n", buf);
  }

  return inst->mcycles();
}

}  // namespace gbemu
