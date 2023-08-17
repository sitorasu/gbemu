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
  // レジスタに対する操作を規定するインターフェース。
  // UIntTypeはレジスタのビット幅を表す符号無し整数型。
  template <class UIntType>
  class Register {
   public:
    Register(std::string name) : name_(name) {}
    virtual UIntType get() = 0;
    virtual void set(UIntType data) = 0;
    std::string name() { return name_; }

   private:
    std::string name_;
  };
  // 単一のレジスタの実装。
  // UIntTypeはレジスタのビット幅を表す符号無し整数型。
  template <class UIntType>
  class SingleRegister : public Register<UIntType> {
   public:
    SingleRegister(std::string name) : Register<UIntType>(name) {}
    UIntType get() override { return data_; }
    void set(UIntType data) override { data_ = data; }

   protected:
    UIntType data_{};
  };
  // レジスタのペアの実装
  // SubUIntはペアを構成するサブレジスタのビット幅を表す符号無し整数型。
  // SuperUIntは全体のビット幅を表す符号無し整数型。
  template <class SubUInt, class SuperUInt>
  class RegisterPair : public Register<SuperUInt> {
    static_assert(sizeof(SuperUInt) == sizeof(SubUInt) * 2,
                  "The size of the pair register must be twice the size of the "
                  "sub register.");

   public:
    // 名前を指定しなければ上位と下位のサブレジスタの名前を結合した名前とする。
    RegisterPair(Register<SubUInt>& upper, Register<SubUInt>& lower)
        : Register<SuperUInt>(upper.name() + lower.name()),
          upper_(upper),
          lower_(lower) {}
    // 名前を明示的に指定することもできる。
    RegisterPair(Register<SubUInt>& upper, Register<SubUInt>& lower,
                 std::string name)
        : Register<SuperUInt>(name), upper_(upper), lower_(lower) {}
    SuperUInt get() override {
      return (static_cast<SuperUInt>(upper_.get()) << (sizeof(SubUInt))) |
             lower_.get();
    }
    void set(SuperUInt data) override {
      lower_.set(static_cast<SubUInt>(data));
      upper_.set(data >> sizeof(SubUInt));
    }

   private:
    Register<SubUInt>& upper_{};
    Register<SubUInt>& lower_{};
  };
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
  };

  using Register8Pair = RegisterPair<std::uint8_t, std::uint16_t>;

  class Registers {
   public:
    SingleRegister<std::uint8_t>& GetRegister8(unsigned i);

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
    SingleRegister<std::uint16_t> pc{"cp"};
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

  // nop
  class Nop : public Instruction {
   public:
    Nop(std::uint16_t address)
        : Instruction(std::vector<std::uint8_t>{0x00}, address, 1, 1) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;
  };

  // jp u16
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

  // di
  class Di : public Instruction {
   public:
    Di(std::uint16_t address)
        : Instruction(std::vector<std::uint8_t>{0xF3}, address, 1, 1) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;
  };

  // ld r16, u16
  class LdR16U16 : public Instruction {
   public:
    LdR16U16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
             Register<std::uint16_t>& reg, std::uint16_t imm)
        : Instruction(std::move(raw_code), address, 3, 4),
          reg_(reg),
          imm_(imm) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;

   private:
    Register<std::uint16_t>& reg_;
    std::uint16_t imm_;
  };

  // ld (u16), a
  class LdA16Ra : public Instruction {
   public:
    LdA16Ra(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
            std::uint16_t imm)
        : Instruction(std::move(raw_code), address, 3, 4), imm_(imm) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;

   private:
    std::uint16_t imm_;
  };

  // ld r8, u8
  class LdR8U8 : public Instruction {
   public:
    LdR8U8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
           Register<std::uint8_t>& reg, std::uint8_t imm)
        : Instruction(std::move(raw_code), address, 2, 2),
          reg_(reg),
          imm_(imm) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;

   private:
    Register<std::uint8_t>& reg_;
    std::uint8_t imm_;
  };

  // ld (FF00+u8), a
  class LdhA8Ra : public Instruction {
   public:
    LdhA8Ra(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
            std::uint8_t imm)
        : Instruction(std::move(raw_code), address, 2, 3), imm_(imm) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;

   private:
    std::uint8_t imm_;
  };

  // call u16
  class CallU16 : public Instruction {
   public:
    CallU16(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
            std::uint16_t imm)
        : Instruction(std::move(raw_code), address, 3, 6), imm_(imm) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;

   private:
    std::uint16_t imm_;
  };

  // ld r8, r8
  class LdR8R8 : public Instruction {
   public:
    LdR8R8(std::vector<std::uint8_t>&& raw_code, std::uint16_t address,
           SingleRegister<std::uint8_t>& dst, SingleRegister<std::uint8_t>& src)
        : Instruction(std::move(raw_code), address, 1, 1),
          dst_(dst),
          src_(src) {}
    std::string GetMnemonicString() override;
    void Execute(Cpu& cpu) override;

   private:
    SingleRegister<std::uint8_t>& dst_;
    SingleRegister<std::uint8_t>& src_;
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
  template <class InstType>
  std::shared_ptr<Instruction> FetchNoOperand();
  template <class InstType>
  std::shared_ptr<Instruction> FetchImm16();
  std::shared_ptr<Instruction> FetchLdR16U16();
  std::shared_ptr<Instruction> FetchLdR8U8();
  std::shared_ptr<Instruction> FetchLdhA8Ra();
  std::shared_ptr<Instruction> FetchLdR8R8();

  Registers registers_;
  Memory& memory_;
};

}  // namespace gbemu

#endif  // GBEMU_CPU_H_
