#include "cpu.h"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

#include "command_line.h"
#include "instruction.h"
#include "interrupt.h"
#include "memory.h"
#include "utils.h"

namespace gbemu {

SingleRegister<std::uint8_t>& Cpu::Registers::GetRegister8ByIndex(unsigned i) {
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
      UNREACHABLE("Invalid register index: %u", i);
  }
}

Register<std::uint16_t>& Cpu::Registers::GetRegister16ByIndex(unsigned i) {
  switch (i) {
    case 0:
      return bc;
    case 1:
      return de;
    case 2:
      return hl;
    case 3:
      return sp;
    default:
      UNREACHABLE("Invalid register index: %u", i);
  }
}

bool Cpu::FlagsRegister::GetFlagByIndex(unsigned i) {
  switch (i) {
    case 0:
      return !z_flag();  // NZ
    case 1:
      return z_flag();  // Z
    case 2:
      return !c_flag();  // NC
    case 3:
      return c_flag();  // C
    default:
      UNREACHABLE("Invalid flag index: %u", i);
  }
}

void Cpu::Registers::Print() {
  auto reg_print = [](auto reg) {
    std::printf("%s:\t%04X\n", reg.name().c_str(), reg.get());
  };
  reg_print(af);
  reg_print(bc);
  reg_print(de);
  reg_print(hl);
  reg_print(sp);
  reg_print(pc);
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

// 命令を標準出力する
// 表示例: `$0637 C3 30 04   jp 0x0430`
void PrintInstruction(std::shared_ptr<Instruction> inst) {
  char buf[64];
  std::string raw_code = Join(inst->raw_code());
  std::string mnemonic = inst->GetMnemonicString();
  std::sprintf(buf, "$%04X %s\t%s", inst->address(), raw_code.c_str(),
               mnemonic.c_str());
  std::printf("%s\n", buf);
}

}  // namespace

unsigned Cpu::Step() {
  std::shared_ptr<Instruction> inst = Instruction::Decode(*this);

  // デバッグモードなら命令の情報を表示
  if (options.debug()) {
    PrintInstruction(inst);
  }

  // imeフラグが立っているなら割り込みを確認
  if (registers_.ime) {
    Interrupt::InterruptSource source = interrupt_.GetRequestedInterrupt();
    if (source != Interrupt::kNone) {
      std::uint16_t address = Interrupt::GetInterruptHandlerAddress(source);
      registers_.ime = false;
      interrupt_.ResetIfBit(source);
      std::uint16_t pc = registers_.pc.get();
      std::uint16_t sp = registers_.sp.get();
      memory_.Write16(sp - 2, pc);
      registers_.sp.set(sp - 2);
      registers_.pc.set(address);
      return 5;
    }
  }

  unsigned mcycles = inst->Execute(*this);

  return mcycles;
}

}  // namespace gbemu
