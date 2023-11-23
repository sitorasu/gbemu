#include "memory.h"

#include <cstdint>
#include <iostream>
#include <vector>

#include "command_line.h"
#include "interrupt.h"
#include "utils.h"

namespace gbemu {

namespace {

// ダミーのIOレジスタ
class IORegisters {
 public:
  std::uint8_t Read(std::uint16_t address) { return GetRegAt(address); }
  void Write(std::uint16_t address, std::uint8_t value) {
    GetRegAt(address) = value;
    // --debugフラグがない場合に限り、シリアル出力を標準出力に接続
    // --debugフラグがあるときは接続しない（デバッグ出力と混ざってぐちゃぐちゃになるので）
    if (!options.debug() && address == 0xFF02 && value == 0x81) {
      std::cout << static_cast<unsigned char>(sb_) << std::flush;
    }
  }

 private:
  std::uint8_t& GetRegAt(std::uint16_t address) {
    switch (address) {
      case 0xFF00:
        return joyp_;
      case 0xFF01:
        return sb_;
      case 0xFF02:
        return sc_;
      case 0xFF11:
        return nr11_;
      case 0xFF12:
        return nr12_;
      case 0xFF13:
        return nr13_;
      case 0xFF14:
        return nr14_;
      case 0xFF24:
        return nr50_;
      case 0xFF25:
        return nr51_;
      case 0xFF26:
        return nr52_;
      case 0xFF40:
        return lcdc_;
      case 0xFF42:
        return scy_;
      case 0xFF43:
        return scx_;
      case 0xFF44:
        return ly_;
      case 0xFF47:
        return bgp_;
      default:
        UNREACHABLE("Unknown I/O register: $%04X", address);
    }
  }

  std::uint8_t joyp_{};    // $FF00
  std::uint8_t sb_{};      // $FF01
  std::uint8_t sc_{};      // $FF02
  std::uint8_t nr11_{};    // $FF11
  std::uint8_t nr12_{};    // $FF12
  std::uint8_t nr13_{};    // $FF13
  std::uint8_t nr14_{};    // $FF14
  std::uint8_t nr50_{};    // $FF24
  std::uint8_t nr51_{};    // $FF25
  std::uint8_t nr52_{};    // $FF26
  std::uint8_t lcdc_{};    // $FF40
  std::uint8_t scy_{};     // $FF42
  std::uint8_t scx_{};     // $FF43
  std::uint8_t ly_{0x90};  // $FF44 VBlankの先頭で固定
  std::uint8_t bgp_{};     // $FF47
};

IORegisters io_regs;

// ダミーのVRAM
std::vector<std::uint8_t> vram(1024 * 8);

}  // namespace

void Memory::WriteIORegister(std::uint16_t address, std::uint8_t value) {
  // CGB固有のレジスタへのアクセスはとりあえず無効とする
  if (address >= 0xFF4D) {
    return;
  }
  switch (address) {
    case 0xFF04:
      timer_.reset_div();
      break;
    case 0xFF05:
      timer_.set_tima(value);
      break;
    case 0xFF06:
      timer_.set_tma(value);
      break;
    case 0xFF07:
      timer_.set_tac(value);
      break;
    case 0xFF0F:
      interrupt_.SetIf(value);
      break;
    default:
      io_regs.Write(address, value);
      break;
  }
}

std::uint8_t Memory::ReadIORegister(std::uint16_t address) {
  // CGB固有のレジスタへのアクセスはとりあえず無効とする
  if (address >= 0xFF4D) {
    // 0xFFを返さないとcpu_instrsで実行環境がCGBと判定されてstopが実行されてしまう
    return 0xFF;
  }
  switch (address) {
    case 0xFF04:
      return timer_.div();
    case 0xFF05:
      return timer_.tima();
    case 0xFF06:
      return timer_.tma();
    case 0xFF07:
      return timer_.tac();
    case 0xFF0F:
      return interrupt_.GetIf();
    default:
      return io_regs.Read(address);
  }
}

uint8_t Memory::Read8(std::uint16_t address) {
  if (InRange(address, 0, 0x8000)) {
    // カートリッジからの読み出し
    return cartridge_->mbc().Read8(address);
  } else if (InRange(address, 0x8000, 0xA000)) {
    // VRAMからの読み出し
    // WARN("Read from VRAM is not implemented.");
    return vram.at(address & 0x1FFF);
  } else if (InRange(address, 0xA000, 0xC000)) {
    // External RAMからの読み出し
    return cartridge_->mbc().Read8(address);
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
    return ReadIORegister(address);
  } else if (InRange(address, 0xFF80, 0xFFFE)) {
    // HRAMからの読み出し
    return h_ram_.at(address & 0x007F);
  } else {
    // レジスタIEからの読み出し
    ASSERT(address == 0xFFFF, "Read from unknown address: %d",
           static_cast<int>(address));
    // WARN("Read from register IE is not implemented.");
    return interrupt_.GetIe();
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
    cartridge_->mbc().Write8(address, value);
  } else if (InRange(address, 0x8000, 0xA000)) {
    // VRAMへの書き込み
    // WARN("Write to VRAM is not implemented.");
    vram.at(address & 0x1FFF) = value;
  } else if (InRange(address, 0xA000, 0xC000)) {
    // External RAMへの書き込み
    cartridge_->mbc().Write8(address, value);
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
    WriteIORegister(address, value);
  } else if (InRange(address, 0xFF80, 0xFFFE)) {
    // HRAMへの書き込み
    h_ram_.at(address & 0x007F) = value;
  } else {
    // レジスタIEへの書き込み
    ASSERT(address == 0xFFFF, "Write to unknown address: %d",
           static_cast<int>(address));
    // WARN("Write to register IE is not implemented.");
    interrupt_.SetIe(value);
  }
}

void Memory::Write16(std::uint16_t address, std::uint16_t value) {
  Write8(address, value & 0xFF);
  Write8(address + 1, value >> 8);
}

}  // namespace gbemu
