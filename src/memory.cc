#include "memory.h"

#include <cstdint>
#include <vector>

#include "utils.h"

namespace gbemu {

namespace {

// ダミーのIOレジスタ
class IORegisters {
 public:
  std::uint8_t Read(std::uint16_t address) { return GetRegAt(address); }
  void Write(std::uint16_t address, std::uint8_t value) {
    GetRegAt(address) = value;
  }

 private:
  std::uint8_t& GetRegAt(std::uint16_t address) {
    switch (address) {
      case 0xFF07:
        return tac_;
      case 0xFF0F:
        return if_;
      case 0xFF24:
        return nr50_;
      case 0xFF25:
        return nr51_;
      case 0xFF26:
        return nr52_;
      case 0xFF40:
        return lcdc_;
      case 0xFF44:
        return ly_;
      case 0xFF4F:
        return vbk_;
      case 0xFF68:
        return bcps_bgpi_;
      case 0xFF69:
        return bcpd_bgpd_;
      default:
        UNREACHABLE("Unknown I/O register: $%04X", address);
    }
  }

  std::uint8_t tac_{};        // $FF07
  std::uint8_t if_{};         // $FF0F
  std::uint8_t nr50_{};       // $FF24
  std::uint8_t nr51_{};       // $FF25
  std::uint8_t nr52_{};       // $FF26
  std::uint8_t lcdc_{};       // $FF40
  std::uint8_t ly_{0x90};     // $FF44 VBlankの先頭で固定
  std::uint8_t vbk_{};        // $FF4F
  std::uint8_t bcps_bgpi_{};  // $FF68
  std::uint8_t bcpd_bgpd_{};  // $FF69
};

IORegisters io_regs;

// ダミーのIEレジスタ
std::uint8_t ie;

// ダミーのVRAM
std::vector<std::uint8_t> vram(1024 * 8);

}  // namespace

uint8_t Memory::Read8(std::uint16_t address) {
  if (InRange(address, 0, 0x8000)) {
    // カートリッジからの読み出し
    return cartridge_.mbc().Read8(address);
  } else if (InRange(address, 0x8000, 0xA000)) {
    // VRAMからの読み出し
    // WARN("Read from VRAM is not implemented.");
    return vram.at(address & 0x1FFF);
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
    // WARN("Read from I/O register is not implemented.");
    return io_regs.Read(address);
  } else if (InRange(address, 0xFF80, 0xFFFE)) {
    // HRAMからの読み出し
    return h_ram_.at(address & 0x007F);
  } else {
    // レジスタIEからの読み出し
    ASSERT(address == 0xFFFF, "Read from unknown address: %d",
           static_cast<int>(address));
    // WARN("Read from register IE is not implemented.");
    return ie;
  }
}

std::uint16_t Memory::Read16(std::uint16_t address) {
  std::uint8_t lower = Read8(address);
  std::uint8_t upper = Read8(address + 1);
  return (((std::uint16_t)upper << 8) | lower);
}

std::vector<std::uint8_t> Memory::ReadBytes(std::uint16_t address,
                                            unsigned bytes) {
  std::vector<std::uint8_t> v(bytes);
  for (unsigned i = 0; i < bytes; ++i) {
    v[i] = Read8(address + i);
  }
  return v;
}

void Memory::Write8(std::uint16_t address, std::uint8_t value) {
  if (InRange(address, 0, 0x8000)) {
    // カートリッジへの書き込み
    cartridge_.mbc().Write8(address, value);
  } else if (InRange(address, 0x8000, 0xA000)) {
    // VRAMへの書き込み
    // WARN("Write to VRAM is not implemented.");
    vram.at(address & 0x1FFF) = value;
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
    // WARN("Write to I/O register is not implemented.");
    io_regs.Write(address, value);
  } else if (InRange(address, 0xFF80, 0xFFFE)) {
    // HRAMへの書き込み
    h_ram_.at(address & 0x007F) = value;
  } else {
    // レジスタIEへの書き込み
    ASSERT(address == 0xFFFF, "Write to unknown address: %d",
           static_cast<int>(address));
    // WARN("Write to register IE is not implemented.");
    ie = value;
  }
}

void Memory::Write16(std::uint16_t address, std::uint16_t value) {
  Write8(address, value & 0xFF);
  Write8(address + 1, value >> 8);
}

}  // namespace gbemu
