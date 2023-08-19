#include "memory.h"

#include <cstdint>

#include "utils.h"

namespace gbemu {

uint8_t Memory::Read8(std::uint16_t address) {
  if (InRange(address, 0, 0x8000)) {
    // カートリッジからの読み出し
    return cartridge_.mbc().Read8(address);
  } else if (InRange(address, 0x8000, 0xA000)) {
    // VRAMからの読み出し
    UNREACHABLE("Read from VRAM is not implemented.");
  } else if (InRange(address, 0xA000, 0xC000)) {
    // External RAMからの読み出し
    return cartridge_.mbc().Read8(address);
  } else if (InRange(address, 0xC000, 0xE000)) {
    // Internal RAMからの読み出し
    return internal_ram_.at(address & 0x1FFF);
  } else if (InRange(address, 0xE000, 0xFE00)) {
    // アクセス禁止区間（$C000-DDFFのミラーらしい）
    Error("Read from $C0000-DDFF is prohibited.");
  } else if (InRange(address, 0xFE00, 0xFDA0)) {
    // OAM RAMからの読み出し
    UNREACHABLE("Read from OAM RAM is not implemented.");
  } else if (InRange(address, 0xFEA0, 0xFF00)) {
    // アクセス禁止区間
    Error("Read from $FEA0-FEFF is prohibited.");
  } else if (InRange(address, 0xFF00, 0xFF80)) {
    // I/Oレジスタからの読み出し
    WARN("Read from I/O Registers is not implemented: return 0!");
    return 0;
  } else if (InRange(address, 0xFF80, 0xFFFE)) {
    // HRAMからの読み出し
    return h_ram_.at(address & 0x007F);
  } else {
    // レジスタIEからの読み出し
    ASSERT(address == 0xFFFF, "Read from unknown address: %d",
           static_cast<int>(address));
    UNREACHABLE("Read from register IE is not implemented.");
  }
}

std::uint16_t Memory::Read16(std::uint16_t address) {
  std::uint8_t lower = Read8(address);
  std::uint8_t upper = Read8(address + 1);
  return (((std::uint16_t)upper << 8) | lower);
}

void Memory::Write8(std::uint16_t address, std::uint8_t value) {
  if (InRange(address, 0, 0x8000)) {
    // カートリッジへの書き込み
    cartridge_.mbc().Write8(address, value);
  } else if (InRange(address, 0x8000, 0xA000)) {
    // VRAMへの書き込み
    UNREACHABLE("Write to VRAM is not implemented.");
  } else if (InRange(address, 0xA000, 0xC000)) {
    // External RAMへの書き込み
    cartridge_.mbc().Write8(address, value);
  } else if (InRange(address, 0xC000, 0xE000)) {
    // Internal RAMへの書き込み
    internal_ram_.at(address & 0x1FFF) = value;
  } else if (InRange(address, 0xE000, 0xFE00)) {
    // アクセス禁止区間（$C000-DDFFのミラーらしい）
    Error("Write to $C0000-DDFF is prohibited.");
  } else if (InRange(address, 0xFE00, 0xFDA0)) {
    // OAM RAMへの書き込み
    UNREACHABLE("Write to OAM RAM is not implemented.");
  } else if (InRange(address, 0xFEA0, 0xFF00)) {
    // アクセス禁止区間
    Error("Write to $FEA0-FEFF is prohibited.");
  } else if (InRange(address, 0xFF00, 0xFF80)) {
    // I/Oレジスタへの書き込み
    WARN("Write to I/O register is not implemented: do nothing!");
  } else if (InRange(address, 0xFF80, 0xFFFE)) {
    // HRAMへの書き込み
    h_ram_.at(address & 0x007F) = value;
  } else {
    // レジスタIEへの書き込み
    ASSERT(address == 0xFFFF, "Write to unknown address: %d",
           static_cast<int>(address));
    WARN("Write to register IE is not implemented.");
  }
}

void Memory::Write16(std::uint16_t address, std::uint16_t value) {
  Write8(address, value & 0xFF);
  Write8(address + 1, value >> 8);
}

}  // namespace gbemu
