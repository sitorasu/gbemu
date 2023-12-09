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
      default:
        UNREACHABLE("Unknown I/O register: $%04X", address);
    }
  }

  std::uint8_t joyp_{};  // $FF00
  std::uint8_t sb_{};    // $FF01
  std::uint8_t sc_{};    // $FF02
  std::uint8_t nr11_{};  // $FF11
  std::uint8_t nr12_{};  // $FF12
  std::uint8_t nr13_{};  // $FF13
  std::uint8_t nr14_{};  // $FF14
  std::uint8_t nr50_{};  // $FF24
  std::uint8_t nr51_{};  // $FF25
  std::uint8_t nr52_{};  // $FF26
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
    case 0xFF40:
      ppu_.set_lcdc(value);
      break;
    case 0xFF41:
      ppu_.set_stat(value);
      break;
    case 0xFF42:
      ppu_.set_scy(value);
      break;
    case 0xFF43:
      ppu_.set_scx(value);
      break;
    case 0xFF45:
      ppu_.set_lyc(value);
      break;
    case 0xFF46:
      dma_.StartDma(value);
      break;
    case 0xFF47:
      ppu_.set_bgp(value);
      break;
    case 0xFF48:
      ppu_.set_obp0(value);
      break;
    case 0xFF49:
      ppu_.set_obp1(value);
      break;
    case 0xFF4A:
      ppu_.set_wy(value);
      break;
    case 0xFF4B:
      ppu_.set_wx(value);
      break;
    default:
      io_regs.Write(address, value);
      break;
  }
}

std::uint8_t Memory::ReadIORegister(std::uint16_t address) const {
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
    case 0xFF40:
      return ppu_.lcdc();
    case 0xFF41:
      return ppu_.stat();
    case 0xFF42:
      return ppu_.scy();
    case 0xFF43:
      return ppu_.scx();
    case 0xFF44:
      return ppu_.ly();
      // return 0x90;  // VBlankの先頭で固定
    case 0xFF45:
      return ppu_.lyc();
    case 0xFF46:
      return dma_.dma();
    case 0xFF47:
      return ppu_.bgp();
    case 0xFF48:
      return ppu_.obp0();
    case 0xFF49:
      return ppu_.obp1();
    case 0xFF4A:
      return ppu_.wy();
    case 0xFF4B:
      return ppu_.wx();
    default:
      return io_regs.Read(address);
  }
}

uint8_t Memory::Read8(std::uint16_t address) const {
  if (InRomRange(address)) {
    // カートリッジからの読み出し
    return cartridge_->Read8(address);
  } else if (InVRamRange(address)) {
    // VRAMからの読み出し
    return ppu_.ReadVRam8(address);
  } else if (InExternalRamRange(address)) {
    // External RAMからの読み出し
    return cartridge_->Read8(address);
  } else if (InInternalRamRange(address)) {
    // Internal RAMからの読み出し
    return internal_ram_.at(address & 0x1FFF);
  } else if (InEchoRamRange(address)) {
    // アクセス禁止区間（$C000-DDFFのミラーらしい）
    Error("Read from $C000-DDFF is prohibited.");
  } else if (InOamRange(address)) {
    // OAM RAMからの読み出し
    return ppu_.ReadOam8(address);
  } else if (InNotUsableAreaRange(address)) {
    // アクセス禁止区間
    Error("Read from $FEA0-FEFF is prohibited.");
  } else if (InIORegistersRange(address)) {
    // I/Oレジスタからの読み出し
    return ReadIORegister(address);
  } else if (InHRamRange(address)) {
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

std::uint16_t Memory::Read16(std::uint16_t address) const {
  std::uint8_t lower = Read8(address);
  std::uint8_t upper = Read8(address + 1);
  return (((std::uint16_t)upper << 8) | lower);
}

std::vector<std::uint8_t> Memory::ReadBytes(std::uint16_t address,
                                            unsigned bytes) const {
  std::vector<std::uint8_t> v(bytes);
  for (unsigned i = 0; i < bytes; ++i) {
    v[i] = Read8(address + i);
  }
  return v;
}

void Memory::Write8(std::uint16_t address, std::uint8_t value) {
  if (InRomRange(address)) {
    // カートリッジへの書き込み
    cartridge_->Write8(address, value);
  } else if (InVRamRange(address)) {
    // VRAMへの書き込み
    ppu_.WriteVRam8(address, value);
  } else if (InExternalRamRange(address)) {
    // External RAMへの書き込み
    cartridge_->Write8(address, value);
  } else if (InInternalRamRange(address)) {
    // Internal RAMへの書き込み
    internal_ram_.at(address & 0x1FFF) = value;
  } else if (InEchoRamRange(address)) {
    // アクセス禁止区間（$C000-DDFFのミラーらしい）
    Error("Write to $C000-DDFF is prohibited.");
  } else if (InOamRange(address)) {
    // OAM RAMへの書き込み
    ppu_.WriteOam8(address, value);
  } else if (InNotUsableAreaRange(address)) {
    // アクセス禁止区間
    Error("Write to $FEA0-FEFF is prohibited.");
  } else if (InIORegistersRange(address)) {
    // I/Oレジスタへの書き込み
    WriteIORegister(address, value);
  } else if (InHRamRange(address)) {
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

void Memory::Dma::Run(unsigned mcycles) {
  if (!during_transfer_) {
    return;
  }
  while (mcycles > 0) {
    std::uint8_t value = memory_.Read8(src_address_);
    memory_.Write8(dst_address_, value);
    src_address_++;
    dst_address_++;
    mcycles--;
    if (dst_address_ == kOamEndAddress) {
      // 他は転送開始時にリセットするので今はほっとく
      during_transfer_ = false;
      break;
    }
  }
}

}  // namespace gbemu
