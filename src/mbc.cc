#include "mbc.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include "cartridge_header.h"
#include "utils.h"

namespace gbemu {

class RomOnly : public Mbc {
 public:
  RomOnly(const std::vector<std::uint8_t>& rom, std::vector<std::uint8_t>& ram)
      : Mbc(rom, ram) {}
  ~RomOnly() override = default;

  std::uint8_t Read8(std::uint16_t address) const override {
    return rom_.at(address);
  }
  void Write8(std::uint16_t /* address */, std::uint8_t /* value */) override {
    // 何もしない
  }
};

class Mbc1 : public Mbc {
 public:
  Mbc1(const std::vector<std::uint8_t>& rom, std::vector<std::uint8_t>& ram)
      : Mbc(rom, ram) {}
  ~Mbc1() override = default;

  std::uint8_t Read8(std::uint16_t address) const override {
    if (InRange(address, 0, 0x4000)) {
      std::uint32_t rom_address;
      if (registers_.ram_banking_mode) {
        rom_address = address;
        rom_address |= registers_.ram_bank_number << 19;
        rom_address %= rom_.size();
      } else {
        rom_address = address;
      }
      return rom_.at(rom_address);
    }

    if (InRange(address, 0x4000, 0x8000)) {
      // ROM Bank Numberレジスタが0の場合は1として扱う
      std::uint8_t rom_bank_number = registers_.rom_bank_number;
      if (rom_bank_number == 0) {
        rom_bank_number = 1;
      }

      std::uint32_t rom_address = 0;
      rom_address |= registers_.ram_bank_number << 19;
      rom_address |= rom_bank_number << 14;
      rom_address |= address & 0x3FFF;
      rom_address %= rom_.size();
      return rom_.at(rom_address);
    }

    if (InRange(address, 0xA000, 0xC000)) {
      if (!registers_.ram_enable || ram_.size() == 0) {
        return 0xFF;  // open bus value
      }

      std::uint16_t ram_address = address & 0x1FFF;
      if (registers_.ram_banking_mode) {
        ram_address |= registers_.ram_bank_number << 13;
        ram_address %= ram_.size();
      }

      return ram_.at(ram_address);
    }

    UNREACHABLE("Unknown address: %d", static_cast<int>(address));
  }

  void Write8(std::uint16_t address, std::uint8_t value) override {
    if (InRange(address, 0, 0x2000)) {
      bool ram_enable = (value & 0xF) == 0xA;
      registers_.ram_enable = ram_enable;
      return;
    }

    if (InRange(address, 0x2000, 0x4000)) {
      std::uint8_t rom_bank_number = value & 0x1F;
      registers_.rom_bank_number = rom_bank_number;
      return;
    }

    if (InRange(address, 0x4000, 0x6000)) {
      std::uint8_t ram_bank_number = value & 0x3;
      registers_.ram_bank_number = ram_bank_number;
      return;
    }

    if (InRange(address, 0x6000, 0x8000)) {
      bool ram_banking_mode = value & 0x1;
      registers_.ram_banking_mode = ram_banking_mode;
      return;
    }

    if (InRange(address, 0xA000, 0xC000)) {
      if (!registers_.ram_enable || ram_.size() == 0) {
        return;
      }

      std::uint16_t ram_address = address & 0x1FFF;
      if (registers_.ram_banking_mode) {
        ram_address |= registers_.ram_bank_number << 13;
        ram_address %= ram_.size();
      }

      ram_.at(ram_address) = value;
      return;
    }

    UNREACHABLE("Unknown address: %d", static_cast<int>(address));
  }

 private:
  struct Registers {
    bool ram_enable;
    std::uint8_t rom_bank_number;
    std::uint8_t ram_bank_number;
    bool ram_banking_mode;
  };
  Registers registers_;
};

std::unique_ptr<Mbc> Mbc::Create(CartridgeType type,
                                 const std::vector<std::uint8_t>& rom,
                                 std::vector<std::uint8_t>& ram) {
  switch (type) {
    case CartridgeType::kRomOnly:
      return std::make_unique<RomOnly>(rom, ram);
    case CartridgeType::kMbc1:
    case CartridgeType::kMbc1Ram:
    case CartridgeType::kMbc1RamBattery:
      return std::make_unique<Mbc1>(rom, ram);
    default:
      UNREACHABLE("Unknown cartridge type.");
  }
}

}  // namespace gbemu
