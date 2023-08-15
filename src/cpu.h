#ifndef GBEMU_CPU_H_
#define GBEMU_CPU_H_

#include <cstdint>
#include <memory>
#include <string>

#include "memory.h"

namespace gbemu {

// CPUのレジスタを表すクラス。
// Example:
//     CpuRegisters regs;
//     regs.set_pc(0x100);
class CpuRegisters {
 public:
  std::uint8_t a() { return a_; }
  std::uint8_t b() { return b_; }
  std::uint8_t c() { return c_; }
  std::uint8_t d() { return d_; }
  std::uint8_t e() { return e_; }
  std::uint8_t h() { return h_; }
  std::uint8_t l() { return l_; }
  std::uint16_t af() { return ((std::uint16_t)a_ << 8) | flags_; }
  std::uint16_t bc() { return ((std::uint16_t)b_ << 8) | c_; }
  std::uint16_t de() { return ((std::uint16_t)d_ << 8) | e_; }
  std::uint16_t hl() { return ((std::uint16_t)h_ << 8) | l_; }
  std::uint16_t sp() { return sp_; }
  std::uint16_t pc() { return pc_; }
  bool z_flag() { return flags_ & (1 << 7); }
  bool n_flag() { return flags_ & (1 << 6); }
  bool h_flag() { return flags_ & (1 << 5); }
  bool c_flag() { return flags_ & (1 << 4); }
  void set_a(std::uint8_t value) { a_ = value; }
  void set_b(std::uint8_t value) { b_ = value; }
  void set_c(std::uint8_t value) { c_ = value; }
  void set_d(std::uint8_t value) { d_ = value; }
  void set_e(std::uint8_t value) { e_ = value; }
  void set_h(std::uint8_t value) { h_ = value; }
  void set_l(std::uint8_t value) { l_ = value; }
  void set_af(std::uint16_t value) {
    a_ = value >> 8;
    flags_ = value & 0xFF;
  }
  void set_bc(std::uint16_t value) {
    b_ = value >> 8;
    c_ = value & 0xFF;
  }
  void set_de(std::uint16_t value) {
    d_ = value >> 8;
    e_ = value & 0xFF;
  }
  void set_hl(std::uint16_t value) {
    h_ = value >> 8;
    l_ = value & 0xFF;
  }
  void set_sp(std::uint16_t value) { sp_ = value; }
  void set_pc(std::uint16_t value) { pc_ = value; }
  void set_z_flag() { flags_ |= (1 << 7); }
  void set_n_flag() { flags_ |= (1 << 6); }
  void set_h_flag() { flags_ |= (1 << 5); }
  void set_c_flag() { flags_ |= (1 << 4); }
  void reset_z_flag() { flags_ &= ~(1 << 7); }
  void reset_n_flag() { flags_ &= ~(1 << 6); }
  void reset_h_flag() { flags_ &= ~(1 << 5); }
  void reset_c_flag() { flags_ &= ~(1 << 4); }

 private:
  std::uint8_t a_;
  std::uint8_t b_;
  std::uint8_t c_;
  std::uint8_t d_;
  std::uint8_t e_;
  std::uint8_t h_;
  std::uint8_t l_;
  std::uint8_t flags_;
  std::uint16_t sp_;
  std::uint16_t pc_;
};

// 命令を表すクラス。
// このクラスを継承したクラスで具体的な命令を表す。
class Instruction {
 public:
  Instruction(std::uint32_t raw_code, std::uint16_t address)
      : raw_code_(raw_code), address_(address) {}
  virtual ~Instruction() = default;

  // ニーモニックの文字列を得る。
  virtual std::string GetMnemonicString() = 0;
  std::uint32_t raw_code() { return raw_code_; }
  std::uint16_t address() { return address_; }
  virtual unsigned length() = 0;  // 命令長（単位：バイト）
  virtual unsigned cycles() = 0;  // 命令のサイクル数（単位：T-cycle）

 private:
  std::uint32_t raw_code_;  // 生の機械語命令（リトルエンディアン）
  std::uint16_t address_;  // 命令の配置されているアドレス
};

// CPUを表すクラス。
// Example:
//     std::vector<std::uint8_t> rom = LoadRom();
//     Cartridge cartridge(std::move(rom));
//     Memory memory(cartridge);
//     Cpu cpu(memory);
//     std::uint8_t cycle = cpu.Step();
class Cpu {
 public:
  Cpu(Memory& memory) : registers_(), memory_(memory) {
    registers_.set_pc(0x100);
  }
  // CPUを1命令分進め、経過したクロック数を返す。
  unsigned Step();

 private:
  std::shared_ptr<Instruction> ExecuteInstruction();
  std::shared_ptr<Instruction> ExecutePrefixedInstruction();
  std::shared_ptr<Instruction> ExecuteNop();
  std::shared_ptr<Instruction> ExecuteJpU16();

  CpuRegisters registers_;
  Memory& memory_;
};

}  // namespace gbemu

#endif  // GBEMU_CPU_H_
