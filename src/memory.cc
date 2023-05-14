#include "memory.h"

#include <cassert>
#include <cstdint>

#include "utils.h"

namespace gbemu {

uint8_t Memory::Read8(std::uint16_t address) {
  if (InRange(address, 0, 0x8000)) {
    // カートリッジからの読み出し
    return cartridge_.mbc().Read8(address);
  } else if (InRange(address, 0x8000, 0xA000)) {
    // VRAMからの読み出し
    assert(false);
  } else if (InRange(address, 0xA000, 0xC000)) {
    // External RAMからの読み出し
    return cartridge_.mbc().Read8(address);
  } else if (InRange(address, 0xC000, 0xE000)) {
    // Internal RAMからの読み出し
    return internal_ram_.at(address & 0x1FFF);
  } else if (InRange(address, 0xE000, 0xFE00)) {
    // アクセス禁止区間（$C000-DDFFのミラーらしい）
    assert(false);
  } else if (InRange(address, 0xFE00, 0xFDA0)) {
    // OAM RAMからの読み出し
    assert(false);
  } else if (InRange(address, 0xFEA0, 0xFF00)) {
    // アクセス禁止区間
    assert(false);
  } else if (InRange(address, 0xFF00, 0xFF80)) {
    // I/Oレジスタからの読み出し
    assert(false);
  } else if (InRange(address, 0xFF80, 0xFFFE)) {
    // HRAMからの読み出し
    assert(false);
  } else {
    // レジスタIEからの読み出し
    assert(address == 0xFFFF);
    assert(false);
  }
}

void Memory::Write8(std::uint16_t address, std::uint8_t value) {
  if (InRange(address, 0, 0x8000)) {
    // カートリッジへの書き込み
    cartridge_.mbc().Write8(address, value);
  } else if (InRange(address, 0x8000, 0xA000)) {
    // VRAMへの書き込み
    assert(false);
  } else if (InRange(address, 0xA000, 0xC000)) {
    // External RAMへの書き込み
    cartridge_.mbc().Write8(address, value);
  } else if (InRange(address, 0xC000, 0xE000)) {
    // Internal RAMへの書き込み
    internal_ram_.at(address & 0x1FFF) = value;
  } else if (InRange(address, 0xE000, 0xFE00)) {
    // アクセス禁止区間（$C000-DDFFのミラーらしい）
    assert(false);
  } else if (InRange(address, 0xFE00, 0xFDA0)) {
    // OAM RAMへの書き込み
    assert(false);
  } else if (InRange(address, 0xFEA0, 0xFF00)) {
    // アクセス禁止区間
    assert(false);
  } else if (InRange(address, 0xFF00, 0xFF80)) {
    // I/Oレジスタへの書き込み
    assert(false);
  } else if (InRange(address, 0xFF80, 0xFFFE)) {
    // HRAMへの書き込み
    assert(false);
  } else {
    // レジスタIEへの書き込み
    assert(address == 0xFFFF);
    assert(false);
  }
}

}  // namespace gbemu
