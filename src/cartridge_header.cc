#include "cartridge_header.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "utils.h"

namespace gbemu {

namespace {

// $0143（CGB flag）を取得
CartridgeTarget GetCartridgeTarget(const std::vector<std::uint8_t>& rom) {
  std::uint8_t value = rom[0x143];
  switch (value) {
    case 0x80:
      return CartridgeTarget::kGbAndGbc;
    case 0xC0:
      return CartridgeTarget::kGbc;
    default:
      if (isprint(value) || (value == 0x00)) {
        return CartridgeTarget::kGb;
      } else {
        Error("Invalid cartridge header at $0143: %d.",
              static_cast<int>(value));
      }
  }
}

// ROMの$0134から15または16バイトを取得
std::string GetTitle(const std::vector<std::uint8_t>& rom) {
  auto title_size = GetCartridgeTarget(rom) == CartridgeTarget::kGb ? 16 : 15;
  return std::string(reinterpret_cast<const char*>(&rom[0x134]), title_size);
}

// $0147（Cartridge type）を取得
CartridgeType GetCartridgeType(const std::vector<std::uint8_t>& rom) {
  std::uint8_t value = rom[0x147];
  switch (value) {
    case 0x00:
      return CartridgeType::kRomOnly;
    case 0x01:
      return CartridgeType::kMbc1;
    case 0x03:
      return CartridgeType::kMbc1RamBattery;
    default:
      Error("Invalid cartridge header at $0147: %d.", static_cast<int>(value));
  }
}

// $0148（ROM size）を取得
unsigned GetRomSize(const std::vector<std::uint8_t>& rom) {
  unsigned value = rom[0x148];
  if (0 <= value && value <= 8) {
    return (32 << value);
  }
  Error("Invalid cartridge header at $0148: %d.", static_cast<int>(value));
}

// $0149（RAM size）を取得
unsigned GetRamSize(const std::vector<std::uint8_t>& rom) {
  unsigned value = rom[0x149];
  switch (value) {
    case 0x00:
      return 0;
    case 0x02:
      return 8;
    case 0x03:
      return 32;
    case 0x04:
      return 128;
    case 0x05:
      return 64;
    default:
      Error("Invalid cartridge header at $0149: %d.", static_cast<int>(value));
  }
}

std::string GetCartridgeTargetString(CartridgeTarget target) {
  switch (target) {
    case CartridgeTarget::kGb:
      return "GB";
    case CartridgeTarget::kGbAndGbc:
      return "GB/GBC";
    case CartridgeTarget::kGbc:
      return "GBC";
  }
}

std::string GetCartridgeTypeString(CartridgeType type) {
  switch (type) {
    case CartridgeType::kRomOnly:
      return "ROM Only";
    case CartridgeType::kMbc1:
      return "MBC1";
    case CartridgeType::kMbc1RamBattery:
      return "MBC1+RAM+BATTERY";
    default:
      return "Unknown";
  }
}

}  // namespace

void CartridgeHeader::Parse(const std::vector<std::uint8_t>& rom) {
  auto size = rom.size();
  auto header_end_address = 0x150U;
  if (size < header_end_address) {
    Error("ROM size is too small.");
  }

  title_ = GetTitle(rom);
  target_ = GetCartridgeTarget(rom);
  type_ = GetCartridgeType(rom);
  rom_size_ = GetRomSize(rom);
  ram_size_ = GetRamSize(rom);

  if (HasRam() != (ram_size() != 0)) {
    Error("Ram size is not consistent with cartridge type.");
  }

  Print();
}

bool CartridgeHeader::HasRam() {
  switch (type_) {
    case CartridgeType::kMbc1Ram:
    case CartridgeType::kMbc1RamBattery:
    case CartridgeType::kRomRam:
    case CartridgeType::kRomRamBattery:
    case CartridgeType::kMmm01Ram:
    case CartridgeType::kMmm01RamBattery:
    case CartridgeType::kMbc3TimerRamBattery:
    case CartridgeType::kMbc3Ram:
    case CartridgeType::kMbc3RamBattery:
    case CartridgeType::kMbc5Ram:
    case CartridgeType::kMbc5RamBattery:
    case CartridgeType::kMbc5RumbleRam:
    case CartridgeType::kMbc5RumbleRamBattery:
    case CartridgeType::kMbc7SensorRumbleRamBattery:
    case CartridgeType::kHuc1RamBattery:
      return true;
    default:
      return false;
  }
}

void CartridgeHeader::Print() {
  std::cout << "=== Cartridge Infomation ===\n";
  std::cout << "title: " << title_ << "\n";
  std::cout << "target: " << GetCartridgeTargetString(target_) << "\n";
  std::cout << "type: " << GetCartridgeTypeString(type_) << "\n";
  std::cout << "rom_size: " << rom_size_ << " KiB\n";
  std::cout << "ram_size: " << ram_size_ << " KiB\n";
  std::cout << "============================\n";
}

}  // namespace gbemu
