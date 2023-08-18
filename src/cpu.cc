#include "cpu.h"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

#include "command_line.h"
#include "instruction.h"
#include "memory.h"
#include "utils.h"

namespace gbemu {

SingleRegister<std::uint8_t>& Cpu::Registers::GetRegister8(unsigned i) {
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

Register<std::uint16_t>& Cpu::Registers::GetRegister16(unsigned i) {
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
}  // namespace

unsigned Cpu::Step() {
  std::shared_ptr<Instruction> inst = Instruction::Decode(*this);

  // デバッグモードなら命令の情報を表示
  // 表示例
  // $0637 C3 30 04   jp 0x0430
  if (options.debug()) {
    char buf[64];
    std::string raw_code = Join(inst->raw_code());
    std::string mnemonic = inst->GetMnemonicString();
    std::sprintf(buf, "$%04X %s\t%s", inst->address(), raw_code.c_str(),
                 mnemonic.c_str());
    std::printf("%s\n", buf);
  }

  unsigned mcycles = inst->Execute(*this);

  return mcycles;
}

}  // namespace gbemu
