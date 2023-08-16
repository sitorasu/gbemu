#ifndef GBEMU_CPU_H_
#define GBEMU_CPU_H_

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "memory.h"

namespace gbemu {

// CPUを表すクラス。
// Example:
//   std::vector<std::uint8_t> rom = LoadRom();
//   Cartridge cartridge(std::move(rom));
//   Memory memory(cartridge);
//   Cpu cpu(memory);
//   std::uint8_t cycle = cpu.Step();
class Cpu {
 private:
  // CPUのレジスタ。
  // Uintはレジスタに格納できる値を表す符号なし整数型。
  template <class Uint>
  class Register {
   public:
    Register(std::string name) : name_(name) {}
    Uint get() { return data_; }
    void set(Uint data) { data_ = data; }
    std::string name() { return name_; }

   private:
    Uint data_{};
    std::string name_;
  };

  using Register8 = Register<std::uint8_t>;
  using Register16 = Register<std::uint16_t>;

  // 8ビットレジスタのペア。
  class Register8Pair {
   public:
    Register8Pair(Register8& lower, Register8& upper, std::string name)
        : lower_(lower), upper_(upper), name_(name) {}
    std::uint16_t get() {
      return (static_cast<std::uint16_t>(upper_.get()) << 8) | lower_.get();
    }
    void set(std::uint16_t data) {
      lower_.set(data & 0xFF);
      upper_.set(data >> 8);
    }
    std::string name() { return name_; }

   private:
    Register8& lower_;
    Register8& upper_;
    std::string name_;
  };

  class Registers {
   public:
    bool z_flag() { return flags.get() & (1 << 7); }
    bool n_flag() { return flags.get() & (1 << 6); }
    bool h_flag() { return flags.get() & (1 << 5); }
    bool c_flag() { return flags.get() & (1 << 4); }
    void set_z_flag() { flags.set(flags.get() | (1 << 7)); }
    void set_n_flag() { flags.set(flags.get() | (1 << 6)); }
    void set_h_flag() { flags.set(flags.get() | (1 << 5)); }
    void set_c_flag() { flags.set(flags.get() | (1 << 4)); }
    void reset_z_flag() { flags.set(flags.get() & ~(1 << 7)); }
    void reset_n_flag() { flags.set(flags.get() & ~(1 << 6)); }
    void reset_h_flag() { flags.set(flags.get() & ~(1 << 5)); }
    void reset_c_flag() { flags.set(flags.get() & ~(1 << 4)); }
    Register8& GetRegister8(unsigned i);

    Register8 a{"a"};
    Register8 b{"b"};
    Register8 c{"c"};
    Register8 d{"d"};
    Register8 e{"e"};
    Register8 h{"h"};
    Register8 l{"l"};
    Register8 flags{"flags"};
    Register8Pair af{a, flags, "af"};
    Register8Pair bc{b, c, "bc"};
    Register8Pair de{d, e, "de"};
    Register8Pair hl{h, l, "hl"};
    Register16 sp{"sp"};
    Register16 pc{"cp"};
    bool ime{false};  // BootROM実行後の値はどうなっている？
  };

  // 命令を表すクラス。
  // このクラスを継承したクラスで具体的な命令を表す。
  class Instruction {
   public:
    Instruction(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
                unsigned length, unsigned mcycles)
        : raw_code_(std::move(raw_code)),
          address_(address),
          length_(length),
          mcycles_(mcycles) {}
    virtual ~Instruction() = default;

    // ニーモニックの文字列を得る。
    virtual std::string GetMnemonicString() = 0;
    // 命令を実行する。
    virtual void Execute(Cpu& cpu) = 0;
    std::vector<std::uint8_t>& raw_code() { return raw_code_; }
    std::uint16_t address() { return address_; }
    unsigned length() { return length_; }
    unsigned mcycles() { return mcycles_; }

   private:
    std::vector<std::uint8_t>
        raw_code_;  // 生の機械語命令（リトルエンディアン）
    std::uint16_t address_;  // 命令の配置されているアドレス
    unsigned length_;        // 命令長（単位：バイト）
    unsigned mcycles_;       // 命令のサイクル数（単位：M-cycle）
  };

  class Nop : public Instruction {
   public:
    Nop(std::uint16_t address)
        : Instruction(std::vector<std::uint8_t>{0x00}, address, 1, 1) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;
  };

  class JpU16 : public Instruction {
   public:
    JpU16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
          std::uint16_t imm)
        : Instruction(std::move(raw_code), address, 3, 4), imm_(imm) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;

   private:
    std::uint16_t imm_;
  };

  class Di : public Instruction {
   public:
    Di(std::uint16_t address)
        : Instruction(std::vector<std::uint8_t>{0xF3}, address, 1, 1) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;
  };

  // RegTypeはRegister8PairまたはRegister16
  template <class RegType>
  class LdR16U16 : public Instruction {
   public:
    LdR16U16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
             RegType& reg, std::uint16_t imm)
        : Instruction(std::move(raw_code), address, 3, 4),
          reg_(reg),
          imm_(imm) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;

   private:
    RegType& reg_;
    std::uint16_t imm_;
  };

 public:
  Cpu(Memory& memory) : registers_(), memory_(memory) {
    registers_.pc.set(0x100);
  }
  // CPUを1命令分進め、経過したクロック数（単位：M-cycle）を返す。
  unsigned Step();

 private:
  void Execute(std::shared_ptr<Instruction> inst) { inst->Execute(*this); }
  std::shared_ptr<Instruction> FetchInstruction();
  std::shared_ptr<Instruction> FetchPrefixedInstruction();
  std::shared_ptr<Instruction> FetchNop();
  std::shared_ptr<Instruction> FetchJpU16();
  std::shared_ptr<Instruction> FetchDi();
  std::shared_ptr<Instruction> FetchLdR16U16();

  Registers registers_;
  Memory& memory_;
};

}  // namespace gbemu

#endif  // GBEMU_CPU_H_
