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

Cpu::SingleRegister<std::uint8_t>& Cpu::Registers::GetRegister8(unsigned i) {
  switch (i) {
    case 0:
      return b;
    case 1:
      return c;
    case 2:
      return d;
    case 3:
      return e;
    case 4:
      return h;
    case 5:
      return l;
    // case 6:
    //   (HL) になってることが多いので例外扱い
    case 7:
      return a;
    default:
      UNREACHABLE("Invalid register index: %d", i);
  }
}

std::string Cpu::Nop::GetMnemonicString() { return "nop"; }

void Cpu::Nop::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers_.pc.get();
  cpu.registers_.pc.set(pc + length());
}

std::string Cpu::JpU16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "jp 0x%04X", imm_);
  return std::string(buf);
}

void Cpu::JpU16::Execute(Cpu& cpu) { cpu.registers_.pc.set(imm_); }

std::string Cpu::Di::GetMnemonicString() { return "di"; }

void Cpu::Di::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers_.pc.get();
  cpu.registers_.pc.set(pc + length());
  cpu.registers_.ime = false;
}

std::string Cpu::LdR16U16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld %s, 0x%04X", reg_.name().c_str(), imm_);
  return std::string(buf);
}

void Cpu::LdR16U16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers_.pc.get();
  reg_.set(imm_);
  cpu.registers_.pc.set(pc + length());
}

std::string Cpu::LdA16Ra::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld (0x%04X), a", imm_);
  return std::string(buf);
}

void Cpu::LdA16Ra::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers_.pc.get();
  std::uint8_t a = cpu.registers_.a.get();
  cpu.memory_.Write8(imm_, a);
  cpu.registers_.pc.set(pc + length());
}

std::string Cpu::LdR8U8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld %s, 0x%02X", reg_.name().c_str(), imm_);
  return std::string(buf);
}

void Cpu::LdR8U8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers_.pc.get();
  reg_.set(imm_);
  cpu.registers_.pc.set(pc + length());
}

std::string Cpu::LdhA8Ra::GetMnemonicString() {
  char buf[32];
  std::sprintf(buf, "ld (0xFF00 + 0x%02X), a", imm_);
  return std::string(buf);
}

void Cpu::LdhA8Ra::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers_.pc.get();
  std::uint8_t a = cpu.registers_.a.get();
  cpu.memory_.Write8(0xFF00 + imm_, a);
  cpu.registers_.pc.set(pc + length());
}

std::string Cpu::CallU16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "call 0x%04X", imm_);
  return std::string(buf);
}

void Cpu::CallU16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers_.pc.get();
  std::uint16_t sp = cpu.registers_.sp.get();
  cpu.memory_.Write16(sp - 1, pc);
  cpu.registers_.sp.set(sp + 2);
  cpu.registers_.pc.set(imm_);
}

std::string Cpu::LdR8R8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld %s, %s", dst_.name().c_str(), src_.name().c_str());
  return std::string(buf);
}

void Cpu::LdR8R8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers_.pc.get();
  dst_.set(src_.get());
  cpu.registers_.pc.set(pc + length());
}

std::shared_ptr<Cpu::Instruction> Cpu::FetchPrefixedInstruction() {
  std::uint16_t pc = registers_.pc.get();
  std::uint8_t opcode = memory_.Read8(pc + 1);
  UNREACHABLE("Unknown opcode: CB %02X", opcode);
}

// [opcode]
// <1byte_value>
template <class InstType>
std::shared_ptr<Cpu::Instruction> Cpu::FetchNoOperand() {
  std::uint16_t pc = registers_.pc.get();
  return std::shared_ptr<Instruction>(new InstType(pc));
}

// [opcode]      [imm]
// <1byte_value> <2byte_value>
template <class InstType>
std::shared_ptr<Cpu::Instruction> Cpu::FetchImm16() {
  std::uint16_t pc = registers_.pc.get();
  std::uint8_t opcode = memory_.Read8(pc);
  std::uint16_t imm = memory_.Read16(pc + 1);
  std::vector<std::uint8_t> raw_code{opcode,
                                     static_cast<std::uint8_t>(imm & 0xFF),
                                     static_cast<std::uint8_t>(imm >> 8)};
  return std::shared_ptr<Instruction>(
      new InstType(std::move(raw_code), pc, imm));
}

// [opcode]   [imm]
// 0b00xx0001 <2byte_value>
//     ||
//     00: bc
//     01: de
//     10: hl
//     11: sp
std::shared_ptr<Cpu::Instruction> Cpu::FetchLdR16U16() {
  std::uint16_t pc = registers_.pc.get();
  std::uint8_t opcode = memory_.Read8(pc);
  std::uint16_t imm = memory_.Read16(pc + 1);
  std::vector<std::uint8_t> raw_code{opcode,
                                     static_cast<std::uint8_t>(imm & 0xFF),
                                     static_cast<std::uint8_t>(imm >> 8)};
  unsigned reg_idx = opcode >> 4;
  switch (reg_idx) {
    case 0:
      return std::shared_ptr<Instruction>(
          new LdR16U16(std::move(raw_code), pc, registers_.bc, imm));
    case 1:
      return std::shared_ptr<Instruction>(
          new LdR16U16(std::move(raw_code), pc, registers_.de, imm));
    case 2:
      return std::shared_ptr<Instruction>(
          new LdR16U16(std::move(raw_code), pc, registers_.hl, imm));
    case 3:
      return std::shared_ptr<Instruction>(
          new LdR16U16(std::move(raw_code), pc, registers_.sp, imm));
    default:
      UNREACHABLE("Invalid register index: %d", reg_idx);
  }
}

// [opcode]
// 0b00xxx110
//
// xxx: dst register
std::shared_ptr<Cpu::Instruction> Cpu::FetchLdR8U8() {
  std::uint16_t pc = registers_.pc.get();
  std::uint8_t opcode = memory_.Read8(pc);
  std::uint8_t imm = memory_.Read8(pc + 1);
  unsigned reg_idx = opcode >> 3;
  Register<std::uint8_t>& reg = registers_.GetRegister8(reg_idx);
  std::vector<std::uint8_t> raw_code{opcode, imm};
  return std::shared_ptr<Instruction>(
      new LdR8U8(std::move(raw_code), pc, reg, imm));
}

std::shared_ptr<Cpu::Instruction> Cpu::FetchLdhA8Ra() {
  std::uint16_t pc = registers_.pc.get();
  std::uint8_t opcode = memory_.Read8(pc);
  std::uint8_t imm = memory_.Read8(pc + 1);
  std::vector<std::uint8_t> raw_code{opcode, imm};
  return std::shared_ptr<Instruction>(
      new LdhA8Ra(std::move(raw_code), pc, imm));
}

// [opcode]
// 0b01xxxyyy
//
// xxx: dst register
// yyy: src register
std::shared_ptr<Cpu::Instruction> Cpu::FetchLdR8R8() {
  std::uint16_t pc = registers_.pc.get();
  std::uint8_t opcode = memory_.Read8(pc);
  unsigned src_idx = opcode & 0x07;
  unsigned dst_idx = (opcode >> 3) & 0x07;
  SingleRegister<uint8_t>& src = registers_.GetRegister8(src_idx);
  SingleRegister<uint8_t>& dst = registers_.GetRegister8(dst_idx);
  std::vector<std::uint8_t> raw_code{opcode};
  return std::shared_ptr<Instruction>(
      new LdR8R8(std::move(raw_code), pc, dst, src));
}

std::shared_ptr<Cpu::Instruction> Cpu::FetchInstruction() {
  std::uint16_t pc = registers_.pc.get();
  std::uint8_t opcode = memory_.Read8(pc);
  if (opcode == 0xCB) {
    // プレフィックスありの命令をフェッチ
    return FetchPrefixedInstruction();
  }
  if (opcode == 0x00) {
    return FetchNoOperand<Nop>();
  }
  if (opcode == 0xC3) {
    return FetchImm16<JpU16>();
  }
  if (opcode == 0xF3) {
    return FetchNoOperand<Di>();
  }
  // opcode = 0b00xx0001
  if (((opcode & 0x0F) == 1) && (opcode >> 4 <= 3)) {
    return FetchLdR16U16();
  }
  if (opcode == 0xEA) {
    return FetchImm16<LdA16Ra>();
  }
  // opcode = 0b00xxx110 AND xxx != 0b110
  if (((opcode & 0x07) == 6) && (opcode >> 6 == 0) && (opcode >> 3 != 6)) {
    return FetchLdR8U8();
  }
  if (opcode == 0xE0) {
    return FetchLdhA8Ra();
  }
  if (opcode == 0xCD) {
    return FetchImm16<CallU16>();
  }
  // opcode = 0b01xxxyyy AND xxx != 0b110 AND yyy != 0b110
  if (((opcode >> 6) & 0x03) == 1 && ((opcode >> 3) & 0x07) != 6 &&
      (opcode & 0x07) != 6) {
    return FetchLdR8R8();
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

  Execute(inst);

  return inst->mcycles();
}

}  // namespace gbemu
