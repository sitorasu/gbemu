#include "instruction.h"

#include <array>
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

// オペランドを取らない命令を表すクラスInstTypeのコンストラクタが引数として
// 命令のアドレスだけを受け取るものと仮定しているがそれを強制する仕組みがどこにもない。
// 同様の問題がDecode**という関数全般にある。
template <class InstType>
std::shared_ptr<Instruction> DecodeNoOperand(Cpu& cpu) {
  return std::make_shared<InstType>(cpu.registers().pc.get());
}

// [opcode]      [imm]
// <1byte_value> <2byte_value>
template <class InstType>
std::shared_ptr<Instruction> DecodeImm16(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::vector<std::uint8_t> raw_code = cpu.memory().ReadBytes(pc, 3);
  std::uint16_t imm = ConcatUInt(raw_code[1], raw_code[2]);
  return std::make_shared<InstType>(std::move(raw_code), pc, imm);
}

// [opcode]      [imm]
// <1byte_value> <1byte_value>
template <class InstType>
std::shared_ptr<Instruction> DecodeImm8(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::vector<std::uint8_t> raw_code = cpu.memory().ReadBytes(pc, 2);
  return std::make_shared<InstType>(std::move(raw_code), pc, raw_code[1]);
}

// [opcode]
// 0b**xxx***
//   7...N..0
//
// 第Nビットから上位側3ビットが8ビットレジスタのインデックス
template <class InstType, unsigned N>
std::shared_ptr<Instruction> DecodeR8(Cpu& cpu) {
  static_assert(N <= 5, "Invalid specialization.");
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  unsigned reg_idx = ExtractBits(opcode, N, 3);
  SingleRegister<std::uint8_t>& reg =
      cpu.registers().GetRegister8ByIndex(reg_idx);
  std::vector<std::uint8_t> raw_code{opcode};
  return std::make_shared<InstType>(std::move(raw_code), pc, reg);
}

// [opcode]
// 0b***xx***
//   7...N..0
//
// 第Nビットから上位側2ビットが16ビットレジスタのインデックス
template <class InstType, unsigned N>
std::shared_ptr<Instruction> DecodeR16(Cpu& cpu) {
  static_assert(N <= 5, "Invalid specialization.");
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  unsigned reg_idx = ExtractBits(opcode, N, 2);
  Register<std::uint16_t>& reg = cpu.registers().GetRegister16ByIndex(reg_idx);
  std::vector<std::uint8_t> raw_code{opcode};
  return std::make_shared<InstType>(std::move(raw_code), pc, reg);
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
  unsigned reg_idx = ExtractBits(opcode, 4, 2);
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
  unsigned reg_idx = ExtractBits(opcode, 3, 3);
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
  unsigned src_idx = ExtractBits(opcode, 0, 3);
  unsigned dst_idx = ExtractBits(opcode, 3, 3);
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
  unsigned reg_idx = ExtractBits(opcode, 4, 2);
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
  unsigned reg_idx = ExtractBits(opcode, 4, 2);
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

// [opcode]   [imm]
// 0b001cc000 <1byte_value>
//
// cc: condition
std::shared_ptr<Instruction> DecodeJrCondS8(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  unsigned cond_idx = ExtractBits(opcode, 3, 2);
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
  std::vector<std::uint8_t> raw_code =
      cpu.memory().ReadBytes(pc, CallCondU16::length);
  std::uint8_t opcode = raw_code[0];
  unsigned cond_idx = ExtractBits(opcode, 3, 2);
  bool cond = cpu.registers().flags.GetFlagByIndex(cond_idx);
  std::uint16_t imm = ConcatUInt(raw_code[1], raw_code[2]);
  return std::make_shared<CallCondU16>(std::move(raw_code), pc, cond, imm);
}

// [opcode]
// 0b110cc000
//
// cc: condition
std::shared_ptr<Instruction> DecodeRetCond(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  unsigned cond_idx = ExtractBits(opcode, 3, 2);
  bool cond = cpu.registers().flags.GetFlagByIndex(cond_idx);
  return std::make_shared<RetCond>(std::vector<std::uint8_t>{opcode}, pc, cond);
}

// [opcode]   [imm]
// 0b110cc010 <2byte_value>
//
// cc: condition
std::shared_ptr<Instruction> DecodeJpCondU16(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::vector<std::uint8_t> raw_code =
      cpu.memory().ReadBytes(pc, CallCondU16::length);
  std::uint8_t opcode = raw_code[0];
  unsigned cond_idx = ExtractBits(opcode, 3, 2);
  bool cond = cpu.registers().flags.GetFlagByIndex(cond_idx);
  std::uint16_t imm = ConcatUInt(raw_code[1], raw_code[2]);
  return std::make_shared<JpCondU16>(std::move(raw_code), pc, cond, imm);
}

// [opcode]
// 0b11xxx111
//
// xxx: imm
std::shared_ptr<Instruction> DecodeRst(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::vector<std::uint8_t> raw_code = {cpu.memory().Read8(pc)};
  std::uint8_t opcode = raw_code[0];
  unsigned imm = ExtractBits(opcode, 3, 3);
  return std::make_shared<Rst>(std::move(raw_code), pc, imm);
}

std::shared_ptr<Instruction> DecodeUnprefixedUnknown(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  UNREACHABLE("Unknown opcode: %02X", opcode);
}

// [prefix] [opcode]
// 0xCB     0b**xxx***
//            7...N..0
//
// オペコードの第Nビットから上位側3ビットがレジスタのインデックス
template <class InstType, unsigned N>
std::shared_ptr<Instruction> DecodePrefixedR8(Cpu& cpu) {
  static_assert(N <= 5, "Invalid specialization.");
  std::uint16_t pc = cpu.registers().pc.get();
  std::vector<std::uint8_t> raw_code = cpu.memory().ReadBytes(pc, 2);
  std::uint8_t opcode = raw_code[1];
  unsigned reg_idx = ExtractBits(opcode, N, 3);
  SingleRegister<std::uint8_t>& reg =
      cpu.registers().GetRegister8ByIndex(reg_idx);
  return std::make_shared<InstType>(std::move(raw_code), pc, reg);
}

// [prefix] [opcode]
// 0xCB     0b**yyy**xxx**
//            7...M....N.0
//
// オペコードの第Mビットから上位側3ビットが即値
// オペコードの第Nビットから上位側3ビットがレジスタのインデックス
template <class InstType, unsigned M, unsigned N>
std::shared_ptr<Instruction> DecodePrefixedU3R8(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::vector<std::uint8_t> raw_code = cpu.memory().ReadBytes(pc, 2);
  std::uint8_t opcode = raw_code[1];
  unsigned imm = ExtractBits(opcode, M, 3);
  unsigned reg_idx = ExtractBits(opcode, N, 3);
  SingleRegister<std::uint8_t>& reg =
      cpu.registers().GetRegister8ByIndex(reg_idx);
  return std::make_shared<InstType>(std::move(raw_code), pc, imm, reg);
}

// [prefix] [opcode]
// 0xCB     0b**xxx***
//            7...N..0
//
// オペコードの第Nビットから上位側3ビットが即値
template <class InstType, unsigned N>
std::shared_ptr<Instruction> DecodePrefixedU3(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::vector<std::uint8_t> raw_code = cpu.memory().ReadBytes(pc, 2);
  std::uint8_t opcode = raw_code[1];
  unsigned imm = ExtractBits(opcode, N, 3);
  return std::make_shared<InstType>(std::move(raw_code), pc, imm);
}

std::shared_ptr<Instruction> DecodePrefixedUnknown(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc + 1);
  UNREACHABLE("Unknown opcode: CB %02X", opcode);
}

// 関数ポインタ配列Instruction::unprefixed_instructionsのコンパイル時初期化を行う
constexpr std::array<Instruction::DecodeFunction, 256> InitUnprefixed() {
  std::array<Instruction::DecodeFunction, 256> result{};
  for (int i = 0; i < 256; i++) {
    result[i] = DecodeUnprefixedUnknown;
  }

  result[0x00] = DecodeNoOperand<Nop>;
  result[0x02] = DecodeNoOperand<LdAbcRa>;
  result[0x07] = DecodeNoOperand<Rlca>;
  result[0x08] = DecodeImm16<LdA16Rsp>;
  result[0x0A] = DecodeNoOperand<LdRaAbc>;
  result[0x0F] = DecodeNoOperand<Rrca>;
  result[0x12] = DecodeNoOperand<LdAdeRa>;
  result[0x17] = DecodeNoOperand<Rla>;
  result[0x18] = DecodeImm8<JrS8>;
  result[0x1A] = DecodeNoOperand<LdRaAde>;
  result[0x1F] = DecodeNoOperand<Rra>;
  result[0x22] = DecodeNoOperand<LdAhliRa>;
  result[0x27] = DecodeNoOperand<Daa>;
  result[0x2A] = DecodeNoOperand<LdRaAhli>;
  result[0x2F] = DecodeNoOperand<Cpl>;
  result[0x32] = DecodeNoOperand<LdAhldRa>;
  result[0x34] = DecodeNoOperand<IncAhl>;
  result[0x35] = DecodeNoOperand<DecAhl>;
  result[0x36] = DecodeImm8<LdAhlU8>;
  result[0x37] = DecodeNoOperand<Scf>;
  result[0x3A] = DecodeNoOperand<LdRaAhld>;
  result[0x3F] = DecodeNoOperand<Ccf>;
  result[0x76] = DecodeNoOperand<Halt>;
  result[0x86] = DecodeNoOperand<AddRaAhl>;
  result[0x8E] = DecodeNoOperand<AdcRaAhl>;
  result[0x96] = DecodeNoOperand<SubRaAhl>;
  result[0x9E] = DecodeNoOperand<SbcRaAhl>;
  result[0xA6] = DecodeNoOperand<AndRaAhl>;
  result[0xAE] = DecodeNoOperand<XorRaAhl>;
  result[0xB6] = DecodeNoOperand<OrRaAhl>;
  result[0xBE] = DecodeNoOperand<CpRaAhl>;
  result[0xC3] = DecodeImm16<JpU16>;
  result[0xC6] = DecodeImm8<AddRaU8>;
  result[0xC9] = DecodeNoOperand<Ret>;
  result[0xCD] = DecodeImm16<CallU16>;
  result[0xCE] = DecodeImm8<AdcRaU8>;
  result[0xD6] = DecodeImm8<SubRaU8>;
  result[0xD9] = DecodeNoOperand<Reti>;
  result[0xDE] = DecodeImm8<SbcRaU8>;
  result[0xE0] = DecodeImm8<LdhA8Ra>;
  result[0xE2] = DecodeNoOperand<LdhAcRa>;
  result[0xE6] = DecodeImm8<AndRaU8>;
  result[0xE8] = DecodeImm8<AddRspS8>;
  result[0xE9] = DecodeNoOperand<JpRhl>;
  result[0xEA] = DecodeImm16<LdA16Ra>;
  result[0xEE] = DecodeImm8<XorRaU8>;
  result[0xF0] = DecodeImm8<LdhRaA8>;
  result[0xF2] = DecodeNoOperand<LdhRaAc>;
  result[0xF3] = DecodeNoOperand<Di>;
  result[0xF6] = DecodeImm8<OrRaU8>;
  result[0xF8] = DecodeImm8<LdRhlRspS8>;
  result[0xF9] = DecodeNoOperand<LdRspRhl>;
  result[0xFA] = DecodeImm16<LdRaA16>;
  result[0xFB] = DecodeNoOperand<Ei>;
  result[0xFE] = DecodeImm8<CpRaU8>;

  std::uint8_t opcode = 0;
  for (std::uint8_t i = 0; i < 8; i++) {
    if (i != 0b110) {
      // opcode == 0b00xxx110 AND xxx != 0b110
      opcode = (i << 3) | 0b110;
      result[opcode] = DecodeLdR8U8;

      // opcode == 0b10110xxx AND xxx != 0b110
      opcode = (0b10110 << 3) | i;
      result[opcode] = DecodeR8<OrRaR8, 0>;

      // opcode == 0b00xxx101 AND xxx != 0b110
      opcode = (i << 3) | 0b101;
      result[opcode] = DecodeR8<DecR8, 3>;

      // opcode == 0b01110xxx AND xxx != 0b110
      opcode = (0b01110 << 3) | i;
      result[opcode] = DecodeR8<LdAhlR8, 0>;

      // opcode == 0b00xxx100 AND xxx != 0b110
      opcode = (i << 3) | 0b100;
      result[opcode] = DecodeR8<IncR8, 3>;

      // opcode == 0b10101xxx AND xxx != 0b110
      opcode = (0b10101 << 3) | i;
      result[opcode] = DecodeR8<XorRaR8, 0>;

      // opcode == 0b01xxx110 AND xxx != 0b110
      opcode = (1 << 6) | (i << 3) | 0b110;
      result[opcode] = DecodeR8<LdR8Ahl, 3>;

      // opcode == 0b10010xxx AND xxx != 0b110
      opcode = (0b10010 << 3) | i;
      result[opcode] = DecodeR8<SubRaR8, 0>;

      // opcode == 0b10111xxx AND xxx != 0b110
      opcode = (0b10111 << 3) | i;
      result[opcode] = DecodeR8<CpRaR8, 0>;

      // opcode == 0b10000xxx AND xxx != 0b110
      opcode = (0b10000 << 3) | i;
      result[opcode] = DecodeR8<AddRaR8, 0>;

      // opcode == 0b10001xxx AND xxx != 0b110
      opcode = (0b10001 << 3) | i;
      result[opcode] = DecodeR8<AdcRaR8, 0>;

      // opcode == 0b10011xxx AND xxx != 0b110
      opcode = (0b10011 << 3) | i;
      result[opcode] = DecodeR8<SbcRaR8, 0>;

      // opcode == 0b10100xxx AND xxx != 0b110
      opcode = (0b10100 << 3) | i;
      result[opcode] = DecodeR8<AndRaR8, 0>;
    }

    // opcode == 0b11xxx111
    opcode = (0b11 << 6) | (i << 3) | 0b111;
    result[opcode] = DecodeRst;
  }

  // opcode == 0b01xxxyyy AND xxx != 0b110 AND yyy != 0b110
  for (std::uint8_t i = 0; i < 8; i++) {
    if (i != 0b110) {
      for (std::uint8_t j = 0; j < 8; j++) {
        if (j != 0b110) {
          opcode = (1 << 6) | (i << 3) | j;
          result[opcode] = DecodeLdR8R8;
        }
      }
    }
  }

  for (std::uint8_t i = 0; i < 4; i++) {
    // opcode == 0b00xx0001
    opcode = (i << 4) | 0b0001;
    result[opcode] = DecodeLdR16U16;

    // opcode == 0b11xx0101
    opcode = (0b11 << 6) | (i << 4) | 0b0101;
    result[opcode] = DecodePushR16;

    // opcode == 0b11xx0001
    opcode = (0b11 << 6) | (i << 4) | 0b0001;
    result[opcode] = DecodePopR16;

    // opcode == 0b00xx0011
    opcode = (i << 4) | 0b0011;
    result[opcode] = DecodeR16<IncR16, 4>;

    // opcode == 0b00xx1011
    opcode = (i << 4) | 0b1011;
    result[opcode] = DecodeR16<DecR16, 4>;

    // opcode == 0b001cc000
    opcode = (1 << 5) | (i << 3);
    result[opcode] = DecodeJrCondS8;

    // opcode == 0b110cc100
    opcode = (0b110 << 5) | (i << 3) | 0b100;
    result[opcode] = DecodeCallCondU16;

    // opcode == 0b110cc000
    opcode = (0b110 << 5) | (i << 3);
    result[opcode] = DecodeRetCond;

    // opcode == 0b00xx1001
    opcode = (i << 4) | 0b1001;
    result[opcode] = DecodeR16<AddRhlR16, 4>;

    // opcode == 0b110cc010
    opcode = (0b110 << 5) | (i << 3) | 0b010;
    result[opcode] = DecodeJpCondU16;
  }

  return result;
}

// 関数ポインタ配列Instruction::Prefixed_instructionsのコンパイル時初期化を行う
constexpr std::array<Instruction::DecodeFunction, 256> InitPrefixed() {
  std::array<Instruction::DecodeFunction, 256> result{};
  for (int i = 0; i < 256; ++i) {
    result[i] = DecodePrefixedUnknown;
  }

  result[0x06] = DecodeNoOperand<RlcAhl>;
  result[0x0E] = DecodeNoOperand<RrcAhl>;
  result[0x16] = DecodeNoOperand<RlAhl>;
  result[0x1E] = DecodeNoOperand<RrAhl>;
  result[0x26] = DecodeNoOperand<SlaAhl>;
  result[0x2E] = DecodeNoOperand<SraAhl>;
  result[0x36] = DecodeNoOperand<SwapAhl>;
  result[0x3E] = DecodeNoOperand<SrlAhl>;

  std::uint8_t opcode = 0;
  for (std::uint8_t i = 0; i < 8; i++) {
    if (i != 0b110) {
      // opcode == 0b00111xxx AND xxx != 0b110
      opcode = (0b111 << 3) | i;
      result[opcode] = DecodePrefixedR8<SrlR8, 0>;

      // opcode == 0b00011xxx AND xxx != 0b110
      opcode = (0b11 << 3) | i;
      result[opcode] = DecodePrefixedR8<RrR8, 0>;

      // opcode == 0b00110xxx AND xxx != 0b110
      opcode = (0b110 << 3) | i;
      result[opcode] = DecodePrefixedR8<SwapR8, 0>;

      // opcode == 0b00000xxx AND xxx != 0b110
      opcode = i;
      result[opcode] = DecodePrefixedR8<RlcR8, 0>;

      // opcode == 0b00001xxx AND xxx != 0b110
      opcode = (1 << 3) | i;
      result[opcode] = DecodePrefixedR8<RrcR8, 0>;

      // opcode == 0b00010xxx AND xxx != 0b110
      opcode = (0b10 << 3) | i;
      result[opcode] = DecodePrefixedR8<RlR8, 0>;

      // opcode == 0b00100xxx AND xxx != 0b110
      opcode = (0b100 << 3) | i;
      result[opcode] = DecodePrefixedR8<SlaR8, 0>;

      // opcode == 0b00101xxx AND xxx != 0b110
      opcode = (0b101 << 3) | i;
      result[opcode] = DecodePrefixedR8<SraR8, 0>;

      for (std::uint8_t j = 0; j < 8; j++) {
        // opcode == 0b01yyyxxx AND xxx != 0b110
        opcode = (1 << 6) | (j << 3) | i;
        result[opcode] = DecodePrefixedU3R8<BitU3R8, 3, 0>;

        // opcode == 0b10yyyxxx AND xxx != 0b110
        opcode = (0b10 << 6) | (j << 3) | i;
        result[opcode] = DecodePrefixedU3R8<ResU3R8, 3, 0>;

        // opcode == 0b11yyyxxx AND xxx != 0b110
        opcode = (0b11 << 6) | (j << 3) | i;
        result[opcode] = DecodePrefixedU3R8<SetU3R8, 3, 0>;
      }
    }
  }

  for (std::uint8_t i = 0; i < 8; i++) {
    // opcode == 0b01xxx110
    opcode = (0b01 << 6) | (i << 3) | 0b110;
    result[opcode] = DecodePrefixedU3<BitU3Ahl, 3>;

    // opcode == 0b10xxx110
    opcode = (0b10 << 6) | (i << 3) | 0b110;
    result[opcode] = DecodePrefixedU3<ResU3Ahl, 3>;

    // opcode == 0b11xxx110
    opcode = (0b11 << 6) | (i << 3) | 0b110;
    result[opcode] = DecodePrefixedU3<SetU3Ahl, 3>;
  }

  return result;
}

}  // namespace

std::array<Instruction::DecodeFunction, 256>
    Instruction::unprefixed_instructions = InitUnprefixed();
std::array<Instruction::DecodeFunction, 256>
    Instruction::prefixed_instructions = InitPrefixed();

std::shared_ptr<Instruction> Instruction::Decode(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t opcode = cpu.memory().Read8(pc);
  if (opcode == 0xCB) {
    opcode = cpu.memory().Read8(pc + 1);
    return prefixed_instructions[opcode](cpu);
  } else {
    return unprefixed_instructions[opcode](cpu);
  }
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
  unsigned cond_idx = ExtractBits(raw_code()[0], 3, 2);
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
  std::uint8_t result = a & imm_;

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
  unsigned cond_idx = ExtractBits(raw_code()[0], 3, 2);
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

std::string DecR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "dec %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned DecR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t result = reg_value - 1;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.set_n_flag();

  if ((reg_value & 0x0F) == 0) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string LdAhlR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld (hl), %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned LdAhlR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t address = cpu.registers().hl.get();
  cpu.memory().Write8(address, reg_.get());
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string IncR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "inc %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned IncR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t result = reg_value + 1;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();

  if ((reg_value & 0x0F) == 0x0F) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string LdRaAde::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld a, (de)");
  return std::string(buf);
}

unsigned LdRaAde::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t de = cpu.registers().de.get();
  std::uint8_t value = cpu.memory().Read8(de);
  cpu.registers().a.set(value);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string XorRaR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "xor a, %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned XorRaR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t result = a ^ reg_.get();
  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  cpu.registers().flags.reset_c_flag();
  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string LdAhliRa::GetMnemonicString() { return "ld (hl+), a"; }

unsigned LdAhliRa::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t a = cpu.registers().a.get();
  cpu.memory().Write8(hl, a);
  cpu.registers().hl.set(hl + 1);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string LdAhldRa::GetMnemonicString() { return "ld (hl-), a"; }

unsigned LdAhldRa::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t a = cpu.registers().a.get();
  cpu.memory().Write8(hl, a);
  cpu.registers().hl.set(hl - 1);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string AddRaU8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "add a, 0x%02X", imm_);
  return std::string(buf);
}

unsigned AddRaU8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t result = a + imm_;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();

  if ((a & 0x0F) + (imm_ & 0x0F) > 0x0F) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  if (static_cast<std::uint16_t>(a) + static_cast<std::uint16_t>(imm_) > 0xFF) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string SubRaU8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "sub a, 0x%02X", imm_);
  return std::string(buf);
}

unsigned SubRaU8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t result = a - imm_;

  if (result == 0) {
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

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string LdR8Ahl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld %s, (hl)", reg_.name().c_str());
  return std::string(buf);
}

unsigned LdR8Ahl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  reg_.set(value);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string LdAdeRa::GetMnemonicString() { return "ld (de), a"; }

unsigned LdAdeRa::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t de = cpu.registers().de.get();
  std::uint8_t a = cpu.registers().a.get();
  cpu.memory().Write8(de, a);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string XorRaAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "xor a, (hl)");
  return std::string(buf);
}

unsigned XorRaAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint16_t a = cpu.registers().a.get();
  std::uint8_t result = a ^ value;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  cpu.registers().flags.reset_c_flag();

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string SrlR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "srl %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned SrlR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t result = reg_value >> 1;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();

  if ((reg_value & 1) == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string RrR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rr %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned RrR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  bool c_flag = cpu.registers().flags.c_flag();
  std::uint8_t result = (reg_value >> 1) | (c_flag ? (1 << 7) : 0);

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();

  if ((reg_value & 1) == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string Rra::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rra");
  return std::string(buf);
}

unsigned Rra::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  bool c_flag = cpu.registers().flags.c_flag();
  std::uint8_t result = (a >> 1) | (c_flag ? (1 << 7) : 0);

  cpu.registers().flags.reset_z_flag();
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();

  if ((a & 1) == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string AdcRaU8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "adc a, 0x%02X", imm_);
  return std::string(buf);
}

unsigned AdcRaU8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  bool c_flag = cpu.registers().flags.c_flag();
  std::uint8_t carry = c_flag ? 1 : 0;
  std::uint8_t result = a + imm_ + carry;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();

  if ((a & 0x0F) + (imm_ & 0x0F) + carry > 0x0F) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  std::uint16_t extended_result = static_cast<std::uint16_t>(a) +
                                  static_cast<std::uint16_t>(imm_) +
                                  static_cast<std::uint16_t>(carry);
  if (extended_result > 0xFF) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string RetCond::GetMnemonicString() {
  char buf[16];
  unsigned cond_idx = ExtractBits(raw_code()[0], 3, 2);
  std::sprintf(buf, "ret %s", cond_str[cond_idx]);
  return std::string(buf);
}

unsigned RetCond::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  if (cond_) {
    std::uint16_t address = Pop(cpu);
    cpu.registers().pc.set(address);
    return 5;
  } else {
    cpu.registers().pc.set(pc + length);
    return 2;
  }
}

std::string OrRaAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "or a, (hl)");
  return std::string(buf);
}

unsigned OrRaAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint16_t a = cpu.registers().a.get();
  std::uint8_t result = a | value;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  cpu.registers().flags.reset_c_flag();

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string DecAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "dec (hl)");
  return std::string(buf);
}

unsigned DecAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t result = value - 1;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.set_n_flag();

  if ((value & 0x0F) == 0) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 3;
}

std::string XorRaU8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "xor a, 0x%02X", imm_);
  return std::string(buf);
}

unsigned XorRaU8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t result = a ^ imm_;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  cpu.registers().flags.reset_c_flag();

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string AddRhlR16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "add hl, %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned AddRhlR16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint16_t reg_value = reg_.get();
  std::uint16_t result = hl + reg_value;

  cpu.registers().flags.reset_n_flag();

  if ((hl & 0xFFF) + (reg_value & 0xFFF) > 0xFFF) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  std::uint32_t extended_result =
      static_cast<std::uint32_t>(hl) + static_cast<std::uint32_t>(reg_value);
  if (extended_result > 0xFFFF) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().hl.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string JpRhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "jp hl");
  return std::string(buf);
}

unsigned JpRhl::Execute(Cpu& cpu) {
  std::uint16_t hl = cpu.registers().hl.get();
  cpu.registers().pc.set(hl);
  return 1;
}

std::string SwapR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "swap %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned SwapR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t result = ((reg_value & 0x0F) << 4) | ((reg_value & 0xF0) >> 4);

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  cpu.registers().flags.reset_c_flag();

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string OrRaU8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "or a, 0x%02X", imm_);
  return std::string(buf);
}

unsigned OrRaU8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t result = a | imm_;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  cpu.registers().flags.reset_c_flag();

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string JpCondU16::GetMnemonicString() {
  char buf[16];
  unsigned cond_idx = ExtractBits(raw_code()[0], 3, 2);
  std::sprintf(buf, "jp %s, 0x%04X", cond_str[cond_idx], imm_);
  return std::string(buf);
}

unsigned JpCondU16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  if (cond_) {
    cpu.registers().pc.set(imm_);
    return 4;
  } else {
    cpu.registers().pc.set(pc + length);
    return 3;
  }
}

std::string SubRaR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "sub a, %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned SubRaR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t result = a - reg_value;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.set_n_flag();

  if ((a & 0x0F) < (reg_value & 0x0F)) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  if (a < reg_value) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string LdA16Rsp::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld (0x%04X), sp", imm_);
  return std::string(buf);
}

unsigned LdA16Rsp::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t sp = cpu.registers().sp.get();
  cpu.memory().Write16(imm_, sp);
  cpu.registers().pc.set(pc + length);
  return 5;
}
std::string LdRspRhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld sp, hl");
  return std::string(buf);
}

unsigned LdRspRhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  cpu.registers().sp.set(hl);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string DecR16::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "dec %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned DecR16::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  reg_.set(reg_.get() - 1);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string AddRspS8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "add sp, 0x%02X", imm_);
  return std::string(buf);
}

unsigned AddRspS8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t sp = cpu.registers().sp.get();
  std::uint16_t sext_imm = (imm_ >> 7) ? (0xFF00 | imm_) : imm_;
  std::uint16_t result = sp + sext_imm;

  cpu.registers().flags.reset_z_flag();
  cpu.registers().flags.reset_n_flag();

  if ((sp & 0x0F) + (imm_ & 0x0F) > 0x0F) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  if ((sp & 0xFF) + imm_ > 0xFF) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().sp.set(result);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string LdRhlRspS8::GetMnemonicString() {
  char buf[32];
  std::sprintf(buf, "ld hl, sp + 0x%02X", imm_);
  return std::string(buf);
}

unsigned LdRhlRspS8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t sp = cpu.registers().sp.get();
  std::uint16_t sext_imm = (imm_ >> 7) ? (0xFF00 | imm_) : imm_;
  std::uint16_t result = sp + sext_imm;

  cpu.registers().flags.reset_z_flag();
  cpu.registers().flags.reset_n_flag();

  if ((sp & 0x0F) + (imm_ & 0x0F) > 0x0F) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  if ((sp & 0xFF) + imm_ > 0xFF) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().hl.set(result);
  cpu.registers().pc.set(pc + length);
  return 3;
}

std::string LdAhlU8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld (hl), 0x%02X", imm_);
  return std::string(buf);
}

unsigned LdAhlU8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t address = cpu.registers().hl.get();
  cpu.memory().Write8(address, imm_);
  cpu.registers().pc.set(pc + length);
  return 3;
}

std::string SbcRaU8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "sbc a, 0x%02X", imm_);
  return std::string(buf);
}

unsigned SbcRaU8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  bool c_flag = cpu.registers().flags.c_flag();
  std::uint8_t carry = c_flag ? 1 : 0;
  std::uint8_t result = a - carry - imm_;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.set_n_flag();

  if ((a & 0x0F) < (imm_ & 0x0F) + carry) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  std::uint16_t zext_imm = imm_;
  if (a < zext_imm + carry) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string LdRaAbc::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld a, (bc)");
  return std::string(buf);
}

unsigned LdRaAbc::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t address = cpu.registers().bc.get();
  std::uint8_t value = cpu.memory().Read8(address);
  cpu.registers().a.set(value);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string LdAbcRa::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld (bc), a");
  return std::string(buf);
}

unsigned LdAbcRa::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint16_t address = cpu.registers().bc.get();
  cpu.memory().Write8(address, a);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string LdRaAhld::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ld a, (hl-)");
  return std::string(buf);
}

unsigned LdRaAhld::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t address = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(address);
  cpu.registers().a.set(value);
  cpu.registers().hl.set(address - 1);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string CpRaAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "cp a, (hl)");
  return std::string(buf);
}

unsigned CpRaAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);

  if ((a - value) == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.set_n_flag();

  if ((a & 0x0F) < (value & 0x0F)) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  if (a < value) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string AddRaAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "add a, (hl)");
  return std::string(buf);
}

unsigned AddRaAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t result = a + value;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();

  if ((a & 0x0F) + (value & 0x0F) > 0x0F) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  std::uint16_t zext_value = value;
  if (a + zext_value > 0xFF) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string AdcRaAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "adc a, (hl)");
  return std::string(buf);
}

unsigned AdcRaAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  bool c_flag = cpu.registers().flags.c_flag();
  std::uint8_t carry = c_flag ? 1 : 0;
  std::uint8_t result = a + value + carry;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();

  if ((a & 0x0F) + (value & 0x0F) + carry > 0x0F) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  std::uint16_t extended_result = static_cast<std::uint16_t>(a) +
                                  static_cast<std::uint16_t>(value) +
                                  static_cast<std::uint16_t>(carry);
  if (extended_result > 0xFF) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string SubRaAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "sub a, (hl)");
  return std::string(buf);
}

unsigned SubRaAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t result = a - value;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.set_n_flag();

  if ((a & 0x0F) < (value & 0x0F)) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  if (a < value) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string SbcRaAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "sbc a, (hl)");
  return std::string(buf);
}

unsigned SbcRaAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  bool c_flag = cpu.registers().flags.c_flag();
  std::uint8_t carry = c_flag ? 1 : 0;
  std::uint8_t result = a - carry - value;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.set_n_flag();

  if ((a & 0x0F) < (value & 0x0F) + carry) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  std::uint16_t zext_imm = value;
  if (a < zext_imm + carry) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string AndRaAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "and a, (hl)");
  return std::string(buf);
}

unsigned AndRaAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t result = a & value;

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

std::string IncAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "inc (hl)");
  return std::string(buf);
}

unsigned IncAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t result = value + 1;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();

  if ((value & 0x0F) == 0x0F) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 3;
}

std::string Cpl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "cpl");
  return std::string(buf);
}

unsigned Cpl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  cpu.registers().a.set(~a);
  cpu.registers().flags.set_n_flag();
  cpu.registers().flags.set_h_flag();
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string Scf::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "scf");
  return std::string(buf);
}

unsigned Scf::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  cpu.registers().flags.set_c_flag();
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string Ccf::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "ccf");
  return std::string(buf);
}

unsigned Ccf::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();

  bool c_flag = cpu.registers().flags.c_flag();
  if (c_flag) {
    cpu.registers().flags.reset_c_flag();
  } else {
    cpu.registers().flags.set_c_flag();
  }

  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string CpRaR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "cp a, %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned CpRaR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t reg_value = reg_.get();

  if ((a - reg_value) == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.set_n_flag();

  if ((a & 0x0F) < (reg_value & 0x0F)) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  if (a < reg_value) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string AddRaR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "add a, %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned AddRaR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t result = a + reg_value;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();

  if ((a & 0x0F) + (reg_value & 0x0F) > 0x0F) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  std::uint16_t zext_reg_value = reg_value;
  if (a + zext_reg_value > 0xFF) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string AdcRaR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "adc a, %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned AdcRaR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t reg_value = reg_.get();
  bool c_flag = cpu.registers().flags.c_flag();
  std::uint8_t carry = c_flag ? 1 : 0;
  std::uint8_t result = a + reg_value + carry;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();

  if ((a & 0x0F) + (reg_value & 0x0F) + carry > 0x0F) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  std::uint16_t extended_result = static_cast<std::uint16_t>(a) +
                                  static_cast<std::uint16_t>(reg_value) +
                                  static_cast<std::uint16_t>(carry);
  if (extended_result > 0xFF) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string SbcRaR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "sbc a, %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned SbcRaR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t reg_value = reg_.get();
  bool c_flag = cpu.registers().flags.c_flag();
  std::uint8_t carry = c_flag ? 1 : 0;
  std::uint8_t result = a - carry - reg_value;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.set_n_flag();

  if ((a & 0x0F) < (reg_value & 0x0F) + carry) {
    cpu.registers().flags.set_h_flag();
  } else {
    cpu.registers().flags.reset_h_flag();
  }

  std::uint16_t zext_imm = reg_value;
  if (a < zext_imm + carry) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string AndRaR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "and a, %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned AndRaR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t result = a & reg_value;

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
  return 1;
}

std::string Rlca::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rlca");
  return std::string(buf);
}

unsigned Rlca::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t a_bit7 = a >> 7;
  std::uint8_t result = ((a & 0x7F) << 1) | a_bit7;

  cpu.registers().flags.reset_z_flag();
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  if (a_bit7 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string Rla::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rla");
  return std::string(buf);
}

unsigned Rla::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t a_bit7 = a >> 7;
  std::uint8_t carry = cpu.registers().flags.c_flag() ? 1 : 0;
  std::uint8_t result = ((a & 0x7F) << 1) | carry;

  cpu.registers().flags.reset_z_flag();
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  if (a_bit7 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string Rrca::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rrca");
  return std::string(buf);
}

unsigned Rrca::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  std::uint8_t a_bit0 = a & 1;
  std::uint8_t result = (a >> 1) | (a_bit0 << 7);

  cpu.registers().flags.reset_z_flag();
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  if (a_bit0 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(result);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string RlcR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rlc %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned RlcR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t reg_bit7 = reg_value >> 7;
  std::uint8_t result = ((reg_value & 0x7F) << 1) | reg_bit7;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  if (reg_bit7 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string RrcR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rrc %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned RrcR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t reg_bit0 = reg_value & 1;
  std::uint8_t result = (reg_value >> 1) | (reg_bit0 << 7);

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();

  if (reg_bit0 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string RlR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rl %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned RlR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t reg_bit7 = reg_value >> 7;
  std::uint8_t carry = cpu.registers().flags.c_flag() ? 1 : 0;
  std::uint8_t result = ((reg_value & 0x7F) << 1) | carry;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  if (reg_bit7 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string SlaR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "sla %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned SlaR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t reg_bit7 = reg_value >> 7;
  std::uint8_t result = (reg_value & 0x7F) << 1;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  if (reg_bit7 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string SraR8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "sra %s", reg_.name().c_str());
  return std::string(buf);
}

unsigned SraR8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  std::uint8_t reg_bit0 = reg_value & 1;
  std::uint8_t result = (reg_value >> 1) | (reg_value & 0x80);

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  if (reg_bit0 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string BitU3R8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "bit %d, %s", imm_, reg_.name().c_str());
  return std::string(buf);
}

unsigned BitU3R8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  ASSERT(imm_ <= 7, "Invalid immediate for BitU3R8: %d", imm_);
  std::uint8_t result = reg_value & (1 << imm_);

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.set_h_flag();

  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string ResU3R8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "res %d, %s", imm_, reg_.name().c_str());
  return std::string(buf);
}

unsigned ResU3R8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  ASSERT(imm_ <= 7, "Invalid immediate for ResU3R8: %d", imm_);
  std::uint8_t result = reg_value & ~(1 << imm_);

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string SetU3R8::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "set %d, %s", imm_, reg_.name().c_str());
  return std::string(buf);
}

unsigned SetU3R8::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t reg_value = reg_.get();
  ASSERT(imm_ <= 7, "Invalid immediate for ResU3R8: %d", imm_);
  std::uint8_t result = reg_value | (1 << imm_);

  reg_.set(result);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string RlcAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rlc (hl)");
  return std::string(buf);
}

unsigned RlcAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t value_bit7 = value >> 7;
  std::uint8_t result = ((value & 0x7F) << 1) | value_bit7;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  if (value_bit7 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string RrcAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rrc (hl)");
  return std::string(buf);
}

unsigned RrcAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t value_bit0 = value & 1;
  std::uint8_t result = (value >> 1) | (value_bit0 << 7);

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();

  if (value_bit0 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string RlAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rl (hl)");
  return std::string(buf);
}

unsigned RlAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t value_bit7 = value >> 7;
  std::uint8_t carry = cpu.registers().flags.c_flag() ? 1 : 0;
  std::uint8_t result = ((value & 0x7F) << 1) | carry;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  if (value_bit7 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string RrAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rr (hl)");
  return std::string(buf);
}

unsigned RrAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  bool c_flag = cpu.registers().flags.c_flag();
  std::uint8_t result = (value >> 1) | (c_flag ? (1 << 7) : 0);

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();

  if ((value & 1) == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string SlaAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "sla (hl)");
  return std::string(buf);
}

unsigned SlaAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t bit7 = value >> 7;
  std::uint8_t result = (value & 0x7F) << 1;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  if (bit7 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string SraAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "sra (hl)");
  return std::string(buf);
}

unsigned SraAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t bit0 = value & 1;
  std::uint8_t result = (value >> 1) | (value & 0x80);

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  if (bit0 == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string SwapAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "swap (hl)");
  return std::string(buf);
}

unsigned SwapAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t result = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();
  cpu.registers().flags.reset_c_flag();

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string SrlAhl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "srl (hl)");
  return std::string(buf);
}

unsigned SrlAhl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t result = value >> 1;

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }

  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.reset_h_flag();

  if ((value & 1) == 1) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string BitU3Ahl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "bit %d, (hl)", imm_);
  return std::string(buf);
}

unsigned BitU3Ahl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t result = value & (1 << imm_);

  if (result == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_n_flag();
  cpu.registers().flags.set_h_flag();

  cpu.registers().pc.set(pc + length);
  return 3;
}

std::string ResU3Ahl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "res %d, (hl)", imm_);
  return std::string(buf);
}

unsigned ResU3Ahl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t result = value & ~(1 << imm_);

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string SetU3Ahl::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "set %d, (hl)", imm_);
  return std::string(buf);
}

unsigned SetU3Ahl::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t hl = cpu.registers().hl.get();
  std::uint8_t value = cpu.memory().Read8(hl);
  std::uint8_t result = value | (1 << imm_);

  cpu.memory().Write8(hl, result);
  cpu.registers().pc.set(pc + length);
  return 4;
}

std::string Daa::GetMnemonicString() { return "daa"; }

unsigned Daa::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t a = cpu.registers().a.get();
  bool n_flag = cpu.registers().flags.n_flag();
  bool c_flag = cpu.registers().flags.c_flag();
  bool h_flag = cpu.registers().flags.h_flag();

  if (!n_flag) {
    // 加算の補正

    // 上桁の補正
    if (c_flag || (a > 0x99)) {
      a += 0x60;
      c_flag = true;
    }
    // 下桁の補正
    if (h_flag || ((a & 0x0F) > 0x09)) {
      a += 0x06;
    }
  } else {
    // 減算の補正

    // 上桁の補正
    if (c_flag) {
      a -= 0x60;
    }
    // 下桁の補正
    if (h_flag) {
      a -= 0x06;
    }
  }

  if (a == 0) {
    cpu.registers().flags.set_z_flag();
  } else {
    cpu.registers().flags.reset_z_flag();
  }
  cpu.registers().flags.reset_h_flag();
  if (c_flag) {
    cpu.registers().flags.set_c_flag();
  } else {
    cpu.registers().flags.reset_c_flag();
  }

  cpu.registers().a.set(a);
  cpu.registers().pc.set(pc + length);
  return 1;
}

std::string LdhRaAc::GetMnemonicString() { return "ldh a, (FF00+c)"; }

unsigned LdhRaAc::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t c = cpu.registers().c.get();
  std::uint8_t value = cpu.memory().Read8(0xFF00 + c);
  cpu.registers().a.set(value);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string LdhAcRa::GetMnemonicString() { return "ldh (FF00+c), a"; }

unsigned LdhAcRa::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint8_t c = cpu.registers().c.get();
  std::uint8_t a = cpu.registers().a.get();
  cpu.memory().Write8(0xFF00 + c, a);
  cpu.registers().pc.set(pc + length);
  return 2;
}

std::string Reti::GetMnemonicString() { return "reti"; }

unsigned Reti::Execute(Cpu& cpu) {
  std::uint16_t address = Pop(cpu);
  cpu.registers().pc.set(address);
  cpu.registers().ime = true;
  return 4;
}

std::string Rst::GetMnemonicString() {
  char buf[16];
  std::sprintf(buf, "rst 0x%02X", imm_ << 3);
  return std::string(buf);
}

unsigned Rst::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  std::uint16_t address = imm_ << 3;
  Push(cpu, pc + length);
  cpu.registers().pc.set(address);
  return 4;
}

std::string Ei::GetMnemonicString() { return "ei"; }

unsigned Ei::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  cpu.registers().pc.set(pc + length);
  cpu.registers().ime = true;
  return 1;
}

std::string Halt::GetMnemonicString() { return "halt"; }

unsigned Halt::Execute(Cpu& cpu) {
  std::uint16_t pc = cpu.registers().pc.get();
  cpu.registers().pc.set(pc + length);
  cpu.Halt();
  return 1;
}

}  // namespace gbemu
