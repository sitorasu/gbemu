#include "memory.h"

#include <cstdint>
#include <iostream>
#include <vector>

#include "apu.h"
#include "command_line.h"
#include "interrupt.h"
#include "joypad.h"
#include "ppu.h"
#include "serial.h"
#include "timer.h"
#include "utils.h"

namespace gbemu {

void Memory::WriteIORegister(std::uint16_t address, std::uint8_t value) {
  switch (address) {
    case 0xFF00:
      joypad_.set_p1(value);
      break;
    case 0xFF01:
      serial_.set_sb(value);
      break;
    case 0xFF02:
      serial_.set_sc(value);
      break;
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
    case 0xFF10:
      apu_.set_nr10(value);
      break;
    case 0xFF11:
      apu_.set_nr11(value);
      break;
    case 0xFF12:
      apu_.set_nr12(value);
      break;
    case 0xFF13:
      apu_.set_nr13(value);
      break;
    case 0xFF14:
      apu_.set_nr14(value);
      break;
    case 0xFF16:
      apu_.set_nr21(value);
      break;
    case 0xFF17:
      apu_.set_nr22(value);
      break;
    case 0xFF18:
      apu_.set_nr23(value);
      break;
    case 0xFF19:
      apu_.set_nr24(value);
      break;
    case 0xFF1A:
      apu_.set_nr30(value);
      break;
    case 0xFF1B:
      apu_.set_nr31(value);
      break;
    case 0xFF1C:
      apu_.set_nr32(value);
      break;
    case 0xFF1D:
      apu_.set_nr33(value);
      break;
    case 0xFF1E:
      apu_.set_nr34(value);
      break;
    case 0xFF20:
      apu_.set_nr41(value);
      break;
    case 0xFF21:
      apu_.set_nr42(value);
      break;
    case 0xFF22:
      apu_.set_nr43(value);
      break;
    case 0xFF23:
      apu_.set_nr44(value);
      break;
    case 0xFF24:
      apu_.set_nr50(value);
      break;
    case 0xFF25:
      apu_.set_nr51(value);
      break;
    case 0xFF26:
      apu_.set_nr52(value);
      break;
    case 0xFF30:
    case 0xFF31:
    case 0xFF32:
    case 0xFF33:
    case 0xFF34:
    case 0xFF35:
    case 0xFF36:
    case 0xFF37:
    case 0xFF38:
    case 0xFF39:
    case 0xFF3A:
    case 0xFF3B:
    case 0xFF3C:
    case 0xFF3D:
    case 0xFF3E:
    case 0xFF3F:
      apu_.set_wave_ram(address - 0xFF30, value);
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
    case 0xFF44:
      // LYはRead Only
      break;
    case 0xFF45:
      ppu_.set_lyc(value);
      break;
    case 0xFF46:
      dma_.RequestDma(value);
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
    case 0xFF50:
      is_boot_rom_mapped_ = false;
      break;
    case 0xFF4A:
      ppu_.set_wy(value);
      break;
    case 0xFF4B:
      ppu_.set_wx(value);
      break;
    default:
      SYSWARN("Write to unknown address: 0x%04X", address);
      break;
  }
}

std::uint8_t Memory::ReadIORegister(std::uint16_t address) const {
  switch (address) {
    case 0xFF00:
      return joypad_.get_p1();
    case 0xFF01:
      return serial_.sb();
    case 0xFF02:
      return serial_.sc();
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
    case 0xFF10:
      return apu_.get_nr10();
    case 0xFF11:
      return apu_.get_nr11();
    case 0xFF12:
      return apu_.get_nr12();
    // case 0xFF13:
    //   NR13 is write-only
    case 0xFF14:
      return apu_.get_nr14();
    case 0xFF16:
      return apu_.get_nr21();
    case 0xFF17:
      return apu_.get_nr22();
    // case 0xFF18:
    //   NR23 is write-only
    case 0xFF19:
      return apu_.get_nr24();
    case 0xFF1A:
      return apu_.get_nr30();
    // case 0xFF1B:
    //   NR30 is write-only
    case 0xFF1C:
      return apu_.get_nr32();
    // case 0xFF1D:
    //   NR33 is write-only
    case 0xFF1E:
      return apu_.get_nr34();
    // case 0xFF20:
    //   NR41 is write-only
    case 0xFF21:
      return apu_.get_nr42();
    case 0xFF22:
      return apu_.get_nr43();
    case 0xFF23:
      return apu_.get_nr44();
    case 0xFF24:
      return apu_.get_nr50();
    case 0xFF25:
      return apu_.get_nr51();
    case 0xFF26:
      return apu_.get_nr52();
    case 0xFF30:
    case 0xFF31:
    case 0xFF32:
    case 0xFF33:
    case 0xFF34:
    case 0xFF35:
    case 0xFF36:
    case 0xFF37:
    case 0xFF38:
    case 0xFF39:
    case 0xFF3A:
    case 0xFF3B:
    case 0xFF3C:
    case 0xFF3D:
    case 0xFF3E:
    case 0xFF3F:
      return apu_.get_wave_ram(address - 0xFF30);
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
      SYSWARN("Read from unknown address: 0x%04X", address);
      return 0xFF;
  }
}

uint8_t Memory::Read8(std::uint16_t address) const {
  if (InRomRange(address)) {
    if (is_boot_rom_mapped_ && address < 0x100) {
      return boot_rom_->at(address);
    } else {
      return cartridge_->Read8(address);
    }
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
    // $C000-DDFFのミラー
    return Read8(address - 0x2000);
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
    // $C000-DDFFのミラー
    Write8(address - 0x2000, value);
  } else if (InOamRange(address)) {
    // OAM RAMへの書き込み
    ppu_.WriteOam8(address, value);
  } else if (InNotUsableAreaRange(address)) {
    // アクセス禁止区間
    SYSWARN("Write to $FEA0-FEFF is prohibited.");
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
    interrupt_.SetIe(value);
  }
}

void Memory::Write16(std::uint16_t address, std::uint16_t value) {
  Write8(address, value & 0xFF);
  Write8(address + 1, value >> 8);
}

void Memory::Dma::Run(unsigned mcycles) {
  if (state_ == State::kWaiting) {
    return;
  }
  if (state_ == State::kRequested) {
    state_ = State::kRunning;
    return;
  }
  while (mcycles > 0) {
    std::uint8_t value = memory_.Read8(src_address_);
    ppu_.WriteOam8WithoutCheck(dst_address_, value);
    src_address_++;
    dst_address_++;
    mcycles--;
    if (dst_address_ == kOamEndAddress) {
      // 他は転送開始時にリセットするので今はほっとく
      state_ = State::kWaiting;
      break;
    }
  }
}

}  // namespace gbemu
