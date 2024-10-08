#ifndef GBEMU_INSTRUCTION_H_
#define GBEMU_INSTRUCTION_H_

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "cpu.h"
#include "register.h"

namespace gbemu {

// 命令を表すクラス。
// このクラスを継承したクラスで具体的な命令を表す。
class Instruction {
 public:
  // 命令デコード用関数の型
  using DecodeFunction = std::shared_ptr<Instruction> (*)(Cpu&);

  Instruction(std::vector<std::uint8_t>&& raw_code, std::uint16_t address)
      : raw_code_(std::move(raw_code)), address_(address) {}
  virtual ~Instruction() = default;

  // 命令を実行し、経過したサイクル数（単位：M-cycle）を返す。
  // 実行の結果によってサイクル数が異なる命令があるので、サイクル数は定数にできない。
  virtual unsigned Execute(Cpu& cpu) = 0;
  // ニーモニックの文字列を得る。
  virtual std::string GetMnemonicString() = 0;
  // プログラムカウンタの位置の命令をデコードする。
  static std::shared_ptr<Instruction> Decode(Cpu& cpu);
  std::vector<std::uint8_t>& raw_code() { return raw_code_; }
  std::uint16_t address() { return address_; }

 private:
  // 生の機械語命令（リトルエンディアン）
  std::vector<std::uint8_t> raw_code_;
  // 命令の配置されているアドレス
  std::uint16_t address_;

  // プレフィックスなしの命令をデコードする関数へのポインタからなる配列。
  // i番目の要素はiをオペコードとする命令をデコードする関数へのポインタ。
  static std::array<DecodeFunction, 256> unprefixed_instructions;
  // プレフィックスありの命令をデコードする関数へのポインタからなる配列。
  // i番目の要素はiを(0xCBに続く)オペコードとする命令をデコードする関数へのポインタ。
  static std::array<DecodeFunction, 256> prefixed_instructions;
};

// nop
class Nop : public Instruction {
 public:
  Nop(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x00}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// jp u16
class JpU16 : public Instruction {
 public:
  JpU16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
        std::uint16_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{3};

 private:
  std::uint16_t imm_;
};

// di
class Di : public Instruction {
 public:
  Di(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xF3}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// ld r16, u16
class LdR16U16 : public Instruction {
 public:
  LdR16U16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
           Register<std::uint16_t>& reg, std::uint16_t imm)
      : Instruction(std::move(raw_code), address), reg_(reg), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{3};

 private:
  Register<std::uint16_t>& reg_;
  std::uint16_t imm_;
};

// ld (u16), a
class LdA16Ra : public Instruction {
 public:
  LdA16Ra(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint16_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{3};

 private:
  std::uint16_t imm_;
};

// ld r8, u8
class LdR8U8 : public Instruction {
 public:
  LdR8U8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
         Register<std::uint8_t>& reg, std::uint8_t imm)
      : Instruction(std::move(raw_code), address), reg_(reg), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  Register<std::uint8_t>& reg_;
  std::uint8_t imm_;
};

// ld (FF00+u8), a
class LdhA8Ra : public Instruction {
 public:
  LdhA8Ra(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// call u16
class CallU16 : public Instruction {
 public:
  CallU16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint16_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{3};

 private:
  std::uint16_t imm_;
};

// ld r8, r8
class LdR8R8 : public Instruction {
 public:
  LdR8R8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
         SingleRegister<std::uint8_t>& dst, SingleRegister<std::uint8_t>& src)
      : Instruction(std::move(raw_code), address), dst_(dst), src_(src) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 private:
  SingleRegister<std::uint8_t>& dst_;
  SingleRegister<std::uint8_t>& src_;
};

// jr s8
class JrS8 : public Instruction {
 public:
  JrS8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
       std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// ret
class Ret : public Instruction {
 public:
  Ret(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xC9}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// push
class PushR16 : public Instruction {
 public:
  PushR16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          Register<std::uint16_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 public:
  Register<std::uint16_t>& reg_;
};

// pop
class PopR16 : public Instruction {
 public:
  PopR16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
         Register<std::uint16_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 public:
  Register<std::uint16_t>& reg_;
};

// inc r16
class IncR16 : public Instruction {
 public:
  IncR16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
         Register<std::uint16_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 public:
  Register<std::uint16_t>& reg_;
};

// ld a, (hl+)
class LdRaAhli : public Instruction {
 public:
  LdRaAhli(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x2A}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// or a, r8
class OrRaR8 : public Instruction {
 public:
  OrRaR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
         SingleRegister<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 private:
  SingleRegister<std::uint8_t>& reg_;
};

// jr cond, s8
class JrCondS8 : public Instruction {
 public:
  JrCondS8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
           bool cond, std::uint8_t imm)
      : Instruction(std::move(raw_code), address), cond_(cond), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  bool cond_;
  std::uint8_t imm_;
};

// ld a, (FF00+u8)
class LdhRaA8 : public Instruction {
 public:
  LdhRaA8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// cp a, u8
class CpRaU8 : public Instruction {
 public:
  CpRaU8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
         std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// ld a, (u16)
class LdRaA16 : public Instruction {
 public:
  LdRaA16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint16_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{3};

 private:
  std::uint16_t imm_;
};

// and a, u8
class AndRaU8 : public Instruction {
 public:
  AndRaU8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// call cond, u16
class CallCondU16 : public Instruction {
 public:
  CallCondU16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
              bool cond, std::uint16_t imm)
      : Instruction(std::move(raw_code), address), cond_(cond), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{3};

 private:
  bool cond_;
  std::uint16_t imm_;
};

// dec r8
class DecR8 : public Instruction {
 public:
  DecR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
        Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 public:
  Register<std::uint8_t>& reg_;
};

// ld (hl), r8
class LdAhlR8 : public Instruction {
 public:
  LdAhlR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          SingleRegister<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 private:
  SingleRegister<std::uint8_t>& reg_;
};

// inc r8
class IncR8 : public Instruction {
 public:
  IncR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
        Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 public:
  Register<std::uint8_t>& reg_;
};

// ld a, (de)
class LdRaAde : public Instruction {
 public:
  LdRaAde(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x1A}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// xor a, r8
class XorRaR8 : public Instruction {
 public:
  XorRaR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 public:
  Register<std::uint8_t>& reg_;
};

// ld (hl+), a
class LdAhliRa : public Instruction {
 public:
  LdAhliRa(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x22}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// ld (hl-), a
class LdAhldRa : public Instruction {
 public:
  LdAhldRa(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x32}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// add a, u8
class AddRaU8 : public Instruction {
 public:
  AddRaU8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

class SubRaU8 : public Instruction {
 public:
  SubRaU8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// ld r8, (hl)
class LdR8Ahl : public Instruction {
 public:
  LdR8Ahl(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          SingleRegister<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 private:
  SingleRegister<std::uint8_t>& reg_;
};

// ld (de), a
class LdAdeRa : public Instruction {
 public:
  LdAdeRa(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x12}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// xor a, (hl)
class XorRaAhl : public Instruction {
 public:
  XorRaAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xAE}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// srl r8
class SrlR8 : public Instruction {
 public:
  SrlR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
        Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  Register<std::uint8_t>& reg_;
};

// rr r8
class RrR8 : public Instruction {
 public:
  RrR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
       Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  Register<std::uint8_t>& reg_;
};

// rra
class Rra : public Instruction {
 public:
  Rra(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x1F}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// adc a, u8
class AdcRaU8 : public Instruction {
 public:
  AdcRaU8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// ret cond
class RetCond : public Instruction {
 public:
  RetCond(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          bool cond)
      : Instruction(std::move(raw_code), address), cond_(cond) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 private:
  bool cond_;
};

// or a, (hl)
class OrRaAhl : public Instruction {
 public:
  OrRaAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xB6}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// dec (hl)
class DecAhl : public Instruction {
 public:
  DecAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x35}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// xor a, u8
class XorRaU8 : public Instruction {
 public:
  XorRaU8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// add hl, r16
class AddRhlR16 : public Instruction {
 public:
  AddRhlR16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
            Register<std::uint16_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 public:
  Register<std::uint16_t>& reg_;
};

// jp hl
class JpRhl : public Instruction {
 public:
  JpRhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xE9}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// swap r8
class SwapR8 : public Instruction {
 public:
  SwapR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
         Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  Register<std::uint8_t>& reg_;
};

// or a, u8
class OrRaU8 : public Instruction {
 public:
  OrRaU8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
         std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// jp cond, u16
class JpCondU16 : public Instruction {
 public:
  JpCondU16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
            bool cond, std::uint16_t imm)
      : Instruction(std::move(raw_code), address), cond_(cond), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{3};

 private:
  bool cond_;
  std::uint16_t imm_;
};

// sub a, r8
class SubRaR8 : public Instruction {
 public:
  SubRaR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          SingleRegister<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 private:
  SingleRegister<std::uint8_t>& reg_;
};

// ld (u16), sp
class LdA16Rsp : public Instruction {
 public:
  LdA16Rsp(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
           std::uint16_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{3};

 private:
  std::uint16_t imm_;
};

// ld sp, hl
class LdRspRhl : public Instruction {
 public:
  LdRspRhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xF9}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// dec r16
class DecR16 : public Instruction {
 public:
  DecR16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
         Register<std::uint16_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 public:
  Register<std::uint16_t>& reg_;
};

// add sp, s8
class AddRspS8 : public Instruction {
 public:
  AddRspS8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
           std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// ld hl, sp + s8
class LdRhlRspS8 : public Instruction {
 public:
  LdRhlRspS8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
             std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// ld (hl), u8
class LdAhlU8 : public Instruction {
 public:
  LdAhlU8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// sbc a, u8
class SbcRaU8 : public Instruction {
 public:
  SbcRaU8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 private:
  std::uint8_t imm_;
};

// ld a, (bc)
class LdRaAbc : public Instruction {
 public:
  LdRaAbc(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x0A}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// ld (bc), a
class LdAbcRa : public Instruction {
 public:
  LdAbcRa(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x0A}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// ld a, (hl-)
class LdRaAhld : public Instruction {
 public:
  LdRaAhld(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x0A}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// cp a, (hl)
class CpRaAhl : public Instruction {
 public:
  CpRaAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xBE}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// add a, (hl)
class AddRaAhl : public Instruction {
 public:
  AddRaAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x86}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// adc a, (hl)
class AdcRaAhl : public Instruction {
 public:
  AdcRaAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x8E}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// sub a, (hl)
class SubRaAhl : public Instruction {
 public:
  SubRaAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x96}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// sbc a, (hl)
class SbcRaAhl : public Instruction {
 public:
  SbcRaAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x9E}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// and a, (hl)
class AndRaAhl : public Instruction {
 public:
  AndRaAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xA6}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// inc (hl)
class IncAhl : public Instruction {
 public:
  IncAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x34}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// cpl
class Cpl : public Instruction {
 public:
  Cpl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x2F}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// scf
class Scf : public Instruction {
 public:
  Scf(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x37}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// ccf
class Ccf : public Instruction {
 public:
  Ccf(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x3F}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// cp a, r8
class CpRaR8 : public Instruction {
 public:
  CpRaR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
         SingleRegister<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 private:
  SingleRegister<std::uint8_t>& reg_;
};

// add a, r8
class AddRaR8 : public Instruction {
 public:
  AddRaR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          SingleRegister<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 private:
  SingleRegister<std::uint8_t>& reg_;
};

// adc a, r8
class AdcRaR8 : public Instruction {
 public:
  AdcRaR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          SingleRegister<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 private:
  SingleRegister<std::uint8_t>& reg_;
};

// sbc a, r8
class SbcRaR8 : public Instruction {
 public:
  SbcRaR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          SingleRegister<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 private:
  SingleRegister<std::uint8_t>& reg_;
};

// and a, r8
class AndRaR8 : public Instruction {
 public:
  AndRaR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          SingleRegister<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 private:
  SingleRegister<std::uint8_t>& reg_;
};

// rlca
class Rlca : public Instruction {
 public:
  Rlca(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x07}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// rla
class Rla : public Instruction {
 public:
  Rla(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x17}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// rrca
class Rrca : public Instruction {
 public:
  Rrca(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x0F}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// rlc r8
class RlcR8 : public Instruction {
 public:
  RlcR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
        Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  Register<std::uint8_t>& reg_;
};

// rrc r8
class RrcR8 : public Instruction {
 public:
  RrcR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
        Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  Register<std::uint8_t>& reg_;
};

// rl r8
class RlR8 : public Instruction {
 public:
  RlR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
       Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  Register<std::uint8_t>& reg_;
};

// sla r8
class SlaR8 : public Instruction {
 public:
  SlaR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
        Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  Register<std::uint8_t>& reg_;
};

// sra r8
class SraR8 : public Instruction {
 public:
  SraR8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
        Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  Register<std::uint8_t>& reg_;
};

// bit u3, r8
class BitU3R8 : public Instruction {
 public:
  BitU3R8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm, Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), imm_(imm), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  std::uint8_t imm_;
  Register<std::uint8_t>& reg_;
};

// res u3, r8
class ResU3R8 : public Instruction {
 public:
  ResU3R8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm, Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), imm_(imm), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  std::uint8_t imm_;
  Register<std::uint8_t>& reg_;
};

// set u3, r8
class SetU3R8 : public Instruction {
 public:
  SetU3R8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint8_t imm, Register<std::uint8_t>& reg)
      : Instruction(std::move(raw_code), address), imm_(imm), reg_(reg) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  std::uint8_t imm_;
  Register<std::uint8_t>& reg_;
};

// rlc (hl)
class RlcAhl : public Instruction {
 public:
  RlcAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xCB, 0x06}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};
};

// rrc (hl)
class RrcAhl : public Instruction {
 public:
  RrcAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xCB, 0x0E}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};
};

// rl (hl)
class RlAhl : public Instruction {
 public:
  RlAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xCB, 0x16}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};
};

// rr (hl)
class RrAhl : public Instruction {
 public:
  RrAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xCB, 0x1E}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};
};

// sla (hl)
class SlaAhl : public Instruction {
 public:
  SlaAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xCB, 0x26}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};
};

// sra (hl)
class SraAhl : public Instruction {
 public:
  SraAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xCB, 0x2E}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};
};

// swap (hl)
class SwapAhl : public Instruction {
 public:
  SwapAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xCB, 0x36}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};
};

// srl (hl)
class SrlAhl : public Instruction {
 public:
  SrlAhl(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xCB, 0x3E}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};
};

// bit u3, (hl)
class BitU3Ahl : public Instruction {
 public:
  BitU3Ahl(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
           std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  std::uint8_t imm_;
};

// res u3, (hl)
class ResU3Ahl : public Instruction {
 public:
  ResU3Ahl(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
           std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  std::uint8_t imm_;
};

// set u3, (hl)
class SetU3Ahl : public Instruction {
 public:
  SetU3Ahl(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
           std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{2};

 public:
  std::uint8_t imm_;
};

// daa
class Daa : public Instruction {
 public:
  Daa(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x27}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// ldh a, (FF00+c)
class LdhRaAc : public Instruction {
 public:
  LdhRaAc(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xF2}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// ldh (FF00+c), a
class LdhAcRa : public Instruction {
 public:
  LdhAcRa(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xE2}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// reti
class Reti : public Instruction {
 public:
  Reti(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xD9}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// rst n
class Rst : public Instruction {
 public:
  Rst(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
      std::uint8_t imm)
      : Instruction(std::move(raw_code), address), imm_(imm) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};

 public:
  std::uint8_t
      imm_;  // opcodeの第3-5ビット。この値を3ビット左シフトしてジャンプ先アドレスを得る。
};

// ei
class Ei : public Instruction {
 public:
  Ei(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0xFB}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

// halt
class Halt : public Instruction {
 public:
  Halt(std::uint16_t address)
      : Instruction(std::vector<std::uint8_t>{0x76}, address) {}
  std::string GetMnemonicString() override;
  unsigned Execute(Cpu& cpu) override;
  static const unsigned length{1};
};

}  // namespace gbemu

#endif  // GBEMU_INSTRUCTION_H_
