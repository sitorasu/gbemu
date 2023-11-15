#ifndef GBEMU_CPU_H_
#define GBEMU_CPU_H_

#include <cstdint>
#include <string>

#include "memory.h"
#include "register.h"

namespace gbemu {

// CPUを表すクラス。
// Example:
//   std::vector<std::uint8_t> rom = LoadRom();
//   Cartridge cartridge(std::move(rom));
//   Memory memory(cartridge);
//   Cpu cpu(memory);
//   std::uint8_t cycle = cpu.Step();
class Cpu {
 public:
  // フラグレジスタ。
  class FlagsRegister : public SingleRegister<std::uint8_t> {
   public:
    FlagsRegister(std::string name) : SingleRegister(name) {}
    bool z_flag() { return data_ & (1 << 7); }
    bool n_flag() { return data_ & (1 << 6); }
    bool h_flag() { return data_ & (1 << 5); }
    bool c_flag() { return data_ & (1 << 4); }
    void set_z_flag() { data_ |= (1 << 7); }
    void set_n_flag() { data_ |= (1 << 6); }
    void set_h_flag() { data_ |= (1 << 5); }
    void set_c_flag() { data_ |= (1 << 4); }
    void reset_z_flag() { data_ &= ~(1 << 7); }
    void reset_n_flag() { data_ &= ~(1 << 6); }
    void reset_h_flag() { data_ &= ~(1 << 5); }
    void reset_c_flag() { data_ &= ~(1 << 4); }
    bool GetFlagByIndex(unsigned i);
    uint8_t get() override { return data_ & 0xF0; }
  };

  using Register8Pair = RegisterPair<std::uint8_t, std::uint16_t>;

  class Registers {
   public:
    SingleRegister<std::uint8_t>& GetRegister8ByIndex(unsigned i);
    Register<std::uint16_t>& GetRegister16ByIndex(unsigned i);
    void Print();

    SingleRegister<std::uint8_t> a{"a"};
    SingleRegister<std::uint8_t> b{"b"};
    SingleRegister<std::uint8_t> c{"c"};
    SingleRegister<std::uint8_t> d{"d"};
    SingleRegister<std::uint8_t> e{"e"};
    SingleRegister<std::uint8_t> h{"h"};
    SingleRegister<std::uint8_t> l{"l"};
    FlagsRegister flags{"flags"};
    Register8Pair af{a, flags, "af"};
    Register8Pair bc{b, c};
    Register8Pair de{d, e};
    Register8Pair hl{h, l};
    SingleRegister<std::uint16_t> sp{"sp"};
    SingleRegister<std::uint16_t> pc{"pc"};
    bool ime{false};  // BootROM実行後の値はどうなっている？
  };

 public:
  Cpu(Memory& memory, Interrupt& interrupt)
      : registers_(), memory_(memory), interrupt_(interrupt) {
    registers_.pc.set(0x100);
  }
  // CPUを1命令分進め、経過したクロック数（単位：M-cycle）を返す。
  unsigned Step();

  Registers& registers() { return registers_; }
  Memory& memory() { return memory_; }

 private:
  Registers registers_;
  Memory& memory_;
  Interrupt& interrupt_;
};

}  // namespace gbemu

#endif  // GBEMU_CPU_H_
