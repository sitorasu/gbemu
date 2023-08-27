#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_

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
  static std::array<DecodeFunction, 0xFF> unprefixed_instructions;
  // プレフィックスありの命令をデコードする関数へのポインタからなる配列。
  // i番目の要素はiを(0xCBに続く)オペコードとする命令をデコードする関数へのポインタ。
  static std::array<DecodeFunction, 0xFF> prefixed_instructions;
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

}  // namespace gbemu

#endif  // INSTRUCTION_H_
