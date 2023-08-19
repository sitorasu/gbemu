
#include "instruction.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cpu.h"
#include "memory.h"
#include "register.h"
#include "utils.h"

namespace gbemu {

namespace {

const char* cond_str[] = {"nz", "z", "nc", "c"};

void Push(Cpu& cpu, std::uint16_t value) {
  std::uint16_t sp = cpu.registers().sp.get();
  cpu.memory().Write16(sp - 2, value);
  cpu.registers().sp.set(sp - 2);
}

std::uint16_t Pop(Cpu& cpu) {
  std::uint16_t sp = cpu.registers().sp.get();
  std::uint16_t value = cpu.memory().Read16(sp);
  cpu.registers().sp.set(sp + 2);
  return value;
}

// [opcode]      [imm]
// <1byte_value> <2byte_value>
template <class InstType>
std::shared_ptr<Instruction> DecodeImm16(Cpu& cpu) {
  static_assert(InstType::length == 3, "Invalid specialization.");
  std::uint16_t pc = cpu.registers().pc.get();
  std::vector<std::uint8_t> raw_code =
      cpu.memory().ReadBytes(pc, InstType::length);
  std::uint16_t imm = ConcatUInt(raw_code[1], raw_code[2]);
  return std::make_shared<InstType>(std::move(raw_code), pc, imm);
}

// [opcode]      [imm]
// <1byte_value> <1byte_value>
template <class InstType>
std::shared_ptr<Instruction> DecodeImm8(Cpu& cpu) {
  static_assert(InstType::length == 2, "Invalid specialization.");
  std::uint16_t pc = cpu.registers().pc.get();
  std::vector<std::uint8_t> raw_code =
      cpu.memory().ReadBytes(pc, InstType::length);
  return std::make_shared<InstType>(std::move(raw_code), pc, raw_code[1]);
}

// [opcode]   [imm]
// 0b00xx0001 <2byte_value>
//
// xx: dst register
std::shared_ptr<Instruction> DecodeLdR16U16(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::vector<std::uint8_t> raw_code =
      cpu.memory().ReadBytes(pc, LdR16U16::length);
  std::uint8_t opcode = raw_code[0];
  unsigned reg_idx = opcode >> 4;
  Register<std::uint16_t>& reg = cpu.registers().GetRegister16ByIndex(reg_idx);
  std::uint16_t imm = ConcatUInt(raw_code[1], raw_code[2]);
  return std::make_shared<LdR16U16>(std::move(raw_code), pc, reg, imm);
}

// [opcode]
// 0b00xxx110
//
// xxx: dst register
std::shared_ptr<Instruction> DecodeLdR8U8(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  std::uint8_t imm = cpu.memory().Read8(pc + 1);
  unsigned reg_idx = opcode >> 3;
  Register<std::uint8_t>& reg = cpu.registers().GetRegister8ByIndex(reg_idx);
  std::vector<std::uint8_t> raw_code{opcode, imm};
  return std::make_shared<LdR8U8>(std::move(raw_code), pc, reg, imm);
}

// [opcode]
// 0b01xxxyyy
//
// xxx: dst register
// yyy: src register
std::shared_ptr<Instruction> DecodeLdR8R8(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  unsigned src_idx = opcode & 0x07;
  unsigned dst_idx = (opcode >> 3) & 0x07;
  SingleRegister<uint8_t>& src = cpu.registers().GetRegister8ByIndex(src_idx);
  SingleRegister<uint8_t>& dst = cpu.registers().GetRegister8ByIndex(dst_idx);
  std::vector<std::uint8_t> raw_code{opcode};
  return std::make_shared<LdR8R8>(std::move(raw_code), pc, dst, src);
}

// [opcode]
// 0b11xx0101
//     ||
//     00: bc
//     01: de
//     10: hl
//     11: af <= ここがspではないのでGetRegister16()が使えない
std::shared_ptr<Instruction> DecodePushR16(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  unsigned reg_idx = (opcode >> 4) & 0x03;
  Register<std::uint16_t>* reg;
  switch (reg_idx) {
    case 0:
      reg = &cpu.registers().bc;
      break;
    case 1:
      reg = &cpu.registers().de;
      break;
    case 2:
      reg = &cpu.registers().hl;
      break;
    case 3:
      reg = &cpu.registers().af;
      break;
    default:
      UNREACHABLE("Invalid register index: %u", reg_idx);
  }
  std::vector<std::uint8_t> raw_code{opcode};
  return std::make_shared<PushR16>(std::move(raw_code), pc, *reg);
}

// [opcode]
// 0b11xx0001
//     ||
//     00: bc
//     01: de
//     10: hl
//     11: af <= ここがspではないのでGetRegister16()が使えない
std::shared_ptr<Instruction> DecodePopR16(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  unsigned reg_idx = (opcode >> 4) & 0x03;
  Register<std::uint16_t>* reg;
  switch (reg_idx) {
    case 0:
      reg = &cpu.registers().bc;
      break;
    case 1:
      reg = &cpu.registers().de;
      break;
    case 2:
      reg = &cpu.registers().hl;
      break;
    case 3:
      reg = &cpu.registers().af;
      break;
    default:
      UNREACHABLE("Invalid register index: %u", reg_idx);
  }
  std::vector<std::uint8_t> raw_code{opcode};
  return std::make_shared<PopR16>(std::move(raw_code), pc, *reg);
}

// [opcode]
// 0b00xx0011
//
// xx: src/dst register
std::shared_ptr<Instruction> DecodeIncR16(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  unsigned reg_idx = (opcode >> 4) & 0x03;
  Register<std::uint16_t>& reg = cpu.registers().GetRegister16ByIndex(reg_idx);
  std::vector<std::uint8_t> raw_code{opcode};
  return std::make_shared<IncR16>(std::move(raw_code), pc, reg);
}

// [opcode]
// 0b10110xxx
//
// xxx: register
std::shared_ptr<Instruction> DecodeOrRaR8(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  unsigned reg_idx = opcode & 0x07;
  SingleRegister<std::uint8_t>& reg =
      cpu.registers().GetRegister8ByIndex(reg_idx);
  std::vector<std::uint8_t> raw_code{opcode};
  return std::make_shared<OrRaR8>(std::move(raw_code), pc, reg);
}

// [opcode]   [imm]
// 0b001cc000 <1byte_value>
//
// cc: condition
std::shared_ptr<Instruction> DecodeJrCondS8(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  unsigned cond_idx = (opcode >> 3) & 0x03;
  bool cond = cpu.registers().flags.GetFlagByIndex(cond_idx);
  std::uint8_t imm = cpu.memory().Read8(pc + 1);
  std::vector<std::uint8_t> raw_code{opcode, imm};
  return std::make_shared<JrCondS8>(std::move(raw_code), pc, cond, imm);
}

// [opcode]   [imm]
// 0b110cc100 <2byte_value>
//
// cc: condition
std::shared_ptr<Instruction> DecodeCallCondU16(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  unsigned cond_idx = (opcode >> 3) & 0x03;
  bool cond = cpu.registers().flags.GetFlagByIndex(cond_idx);
  std::uint16_t imm = cpu.memory().Read16(pc + 1);
  std::vector<std::uint8_t> raw_code{opcode,
                                     static_cast<std::uint8_t>(imm & 0xFF),
                                     static_cast<std::uint8_t>(imm >> 8)};
  return std::make_shared<CallCondU16>(std::move(raw_code), pc, cond, imm);
}

std::shared_ptr<Instruction> DecodePrefixed(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc + 1);
  UNREACHABLE("Unknown opcode: CB %02X", opcode);
}

}  // namespace

std::shared_ptr<Instruction> Instruction::Decode(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  if (opcode == 0xCB) {
    // プレフィックスありの命令をデコード
    return DecodePrefixed(cpu);
  }
  if (opcode == 0x00) {
    return std::make_shared<Nop>(pc);
  }
  if (opcode == 0xC3) {
    return DecodeImm16<JpU16>(cpu);
  }
  if (opcode == 0xF3) {
    return std::make_shared<Di>(pc);
  }
  // opcode = 0b00xx0001
  if ((opcode & 0x0F) == 0x01 && (opcode >> 4) <= 0x03) {
    return DecodeLdR16U16(cpu);
  }
  if (opcode == 0xEA) {
    return DecodeImm16<LdA16Ra>(cpu);
  }
  // opcode = 0b00xxx110 AND xxx != 0b110
  if ((opcode & 0x07) == 0x06 && (opcode >> 6) == 0x00 &&
      (opcode >> 3) != 0x06) {
    return DecodeLdR8U8(cpu);
  }
  if (opcode == 0xE0) {
    return DecodeImm8<LdhA8Ra>(cpu);
  }
  if (opcode == 0xCD) {
    return DecodeImm16<CallU16>(cpu);
  }
  // opcode = 0b01xxxyyy AND xxx != 0b110 AND yyy != 0b110
  if (((opcode >> 6) & 0x03) == 0x01 && ((opcode >> 3) & 0x07) != 0x06 &&
      (opcode & 0x07) != 0x06) {
    return DecodeLdR8R8(cpu);
  }
  if (opcode == 0x18) {
    return DecodeImm8<JrS8>(cpu);
  }
  if (opcode == 0xC9) {
    return std::make_shared<Ret>(pc);
  }
  // opcode = 0b11xx0101
  if ((opcode & 0x0F) == 0x05 && (opcode >> 6) == 0x03) {
    return DecodePushR16(cpu);
  }
  // opcode = 0b11xx0001
  if ((opcode & 0x0F) == 0x01 && (opcode >> 6) == 0x03) {
    return DecodePopR16(cpu);
  }
  // opcode = 0b00xx0011
  if ((opcode & 0x0F) == 0x03 && (opcode >> 6) == 0x00) {
    return DecodeIncR16(cpu);
  }
  if (opcode == 0x2A) {
    return std::make_shared<LdRaAhli>(pc);
  }
  // opcode = 0b10110xxx
  if ((opcode >> 3) == 0x16 && (opcode & 0x07) != 0x06) {
    return DecodeOrRaR8(cpu);
  }
  // opcode = 0b001cc000
  if ((opcode >> 5) == 0x01 && (opcode & 0x07) == 0x00) {
    return DecodeJrCondS8(cpu);
  }
  if (opcode == 0xF0) {
    return DecodeImm8<LdhRaA8>(cpu);
  }
  if (opcode == 0xFE) {
    return DecodeImm8<CpRaU8>(cpu);
  }
  if (opcode == 0xFA) {
    return DecodeImm16<LdRaA16>(cpu);
  }
  if (opcode == 0xE6) {
    return DecodeImm8<AndRaU8>(cpu);
  }
  // opcode = 0b110cc100
  if ((opcode >> 5) == 0x06 && (opcode & 0x07) == 0x04) {
    return DecodeCallCondU16(cpu);
  }
  UNREACHABLE("Unknown opcode: %02X\n", opcode);
}

std::string Nop::GetMnemonicString() { return "nop"; }

unsigned Nop::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string JpU16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "jp 0x%04X", imm_);
  return std::string(buf);
}

unsigned JpU16::Execute(Cpu& cpu) {
  cpu.registers().pc.set(imm_);
  return 4;
}

std::string Di::GetMnemonicString() { return "di"; }

unsigned Di::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  cpu.registers().pc.set(pc + length);
  cpu.registers().ime = false;
  return 1;
}

std::string LdR16U16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld %s, 0x%04X", reg_.name().c_str(), imm_);
  return std::string(buf);
}

unsigned LdR16U16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  reg_.set(imm_);
  cpu.registers().pc.set(pc + length);
  return 3;
}

std::string LdA16Ra::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld (0x%04X), a", imm_);
  return std::string(buf);
}

unsigned LdA16Ra::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  cpu.memory().Write8(imm_, a);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string LdR8U8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld %s, 0x%02X", reg_.name().c_str(), imm_);
  return std::string(buf);
}

unsigned LdR8U8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  reg_.set(imm_);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string LdhA8Ra::GetMnemonicString() {
  char buf[32];
  std::sprintf(buf, "ld (0xFF00 + 0x%02X), a", imm_);
  return std::string(buf);
}

unsigned LdhA8Ra::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  cpu.memory().Write8(0xFF00 + imm_, a);
  cpu.registers().pc.set(pc + length);
  return 3;
}

std::string CallU16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "call 0x%04X", imm_);
  return std::string(buf);
}

unsigned CallU16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  Push(cpu, pc + length);
  cpu.registers().pc.set(imm_);
  return 6;
}

std::string LdR8R8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld %s, %s", dst_.name().c_str(), src_.name().c_str());
  return std::string(buf);
}

unsigned LdR8R8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  dst_.set(src_.get());
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string JrS8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "jr 0x%02X", imm_);
  return std::string(buf);
}

unsigned JrS8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t disp = (imm_ >> 7) ? 0xFF00 | imm_ : imm_;  // 符号拡張
  cpu.registers().pc.set(pc + length + disp);
  return 3;
}

std::string Ret::GetMnemonicString() { return "ret"; }

unsigned Ret::Execute(Cpu& cpu) {
  std::uint16_t address = Pop(cpu);
  cpu.registers().pc.set(address);
  return 4;
}

std::string PushR16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "push %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned PushR16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  Push(cpu, reg_.get());
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string PopR16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "pop %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned PopR16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  reg_.set(Pop(cpu));
  cpu.registers().pc.set(pc + length);
  return 3;
}

std::string IncR16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "inc %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned IncR16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  reg_.set(reg_.get() + 1);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string LdRaAhli::GetMnemonicString() { return "ld a, (hl+)"; }

unsigned LdRaAhli::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  cpu.registers().a.set(value);
  cpu.registers().hl.set(hl + 1);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string OrRaR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "or a, %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned OrRaR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t result = cpu.registers().a.get() | reg_.get();
  cpu.registers().a.set(result);
  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  cpu.registers().flags.reset_c_flag();
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string JrCondS8::GetMnemonicString() {
  char buf[16];
  unsigned cond_idx = (raw_code()[0] >> 3) & 0x03;
  std::sprintf(buf, "jr %s, 0x%02X", cond_str[cond_idx], imm_);
  return std::string(buf);
}

unsigned JrCondS8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  if (cond_) {
    std::uint16_t disp = (imm_ >> 7) ? 0xFF00 | imm_ : imm_;  // 符号拡張
    cpu.registers().pc.set(pc + length + disp);
    return 3;
  } else {
    cpu.registers().pc.set(pc + length);
    return 2;
  }
}

std::string LdhRaA8::GetMnemonicString() {
  char buf[32];
  std::sprintf(buf, "ld a, (0xFF00 + 0x%02X)", imm_);
  return std::string(buf);
}

unsigned LdhRaA8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t value = cpu.memory().Read8(0xFF00 + imm_);
  cpu.registers().a.set(value);
  cpu.registers().pc.set(pc + length);
  return 3;
}

std::string CpRaU8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "cp a, 0x%02X", imm_);
  return std::string(buf);
}

unsigned CpRaU8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();

  if ((a - imm_) == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.set_n_flag();

  if ((a & 0x0F) < (imm_ & 0x0F)) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  if (a < imm_) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string LdRaA16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld a, (0x%04X)", imm_);
  return std::string(buf);
}

unsigned LdRaA16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t value = cpu.memory().Read8(imm_);
  cpu.registers().a.set(value);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string AndRaU8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "and a, 0x%02X", imm_);
  return std::string(buf);
}

unsigned AndRaU8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t result = a | imm_;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.set_h_flag();
  cpu.registers().flags.reset_c_flag();

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string CallCondU16::GetMnemonicString() {
  char buf[16];
  unsigned cond_idx = (raw_code()[0] >> 3) & 0x03;
  std::sprintf(buf, "call %s, 0x%04X", cond_str[cond_idx], imm_);
  return std::string(buf);
}

unsigned CallCondU16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  if (cond_) {
    Push(cpu, pc + length);
    cpu.registers().pc.set(imm_);
    return 6;
  } else {
    cpu.registers().pc.set(pc + length);
    return 3;
  }
}

}  // namespace gbemu
