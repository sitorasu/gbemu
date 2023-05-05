#include "memory.h"

#include <cassert>

namespace gbemu {

namespace {
// xが区間[begin, end)に含まれるならtrue、そうでなければfalseを返す
template <class T1, class T2, class T3>
bool in_range(T1 x, T2 begin, T3 end) {
  return begin <= x && x < end;
}
}  // namespace

// 後で実装する
Memory::Memory() {}

uint8_t Memory::read(std::uint16_t address) {
  // 後で実装する
  return 42;
}

void Memory::write(std::uint16_t address, std::uint8_t value) {
  if (in_range(address, 0, 0x8000)) {
    // ROMへの書き込み、つまりMBCの操作
  } else if (in_range(address, 0x8000, 0xA000)) {
    // VRAMへの書き込み
  } else if (in_range(address, 0xA000, 0xC000)) {
    // External RAMへの書き込み
  } else if (in_range(address, 0xC000, 0xE000)) {
    // Internal RAMへの書き込み
  } else if (in_range(address, 0xE000, 0xFE00)) {
    // アクセス禁止区間（$C000-DDFFのミラーらしい）
  } else if (in_range(address, 0xFE00, 0xFDA0)) {
    // OAM RAMへの書き込み
  } else if (in_range(address, 0xFEA0, 0xFF00)) {
    // アクセス禁止区間
  } else if (in_range(address, 0xFF00, 0xFF80)) {
    // I/Oレジスタへの書き込み
  } else if (in_range(address, 0xFF80, 0xFFFE)) {
    // HRAMへの書き込み
  } else {
    // レジスタIEへの書き込み
    assert(address == 0xFFFF);
  }
}

}  // namespace gbemu
