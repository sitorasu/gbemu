#include "cartridge_header.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "utils.h"

namespace gbemu {

using CartridgeTarget = CartridgeHeader::CartridgeTarget;
using CartridgeType = CartridgeHeader::CartridgeType;

CartridgeHeader CartridgeHeader::Create(const std::vector<std::uint8_t>& rom) {
  auto size = rom.size();
  constexpr auto kHeaderSize = 0x150;
  if (size < kHeaderSize) {
    Error("ROM size is too small.");
  }
  CartridgeHeader header(rom);
  header.Print();
  return header;
}

// ROMの$0134から15または16バイトを取得
std::string CartridgeHeader::GetTitle(const std::vector<std::uint8_t>& rom) {
  auto title_size = GetCartridgeTarget(rom) == CartridgeTarget::kGb ? 16 : 15;
  return std::string(reinterpret_cast<const char*>(&rom[0x134]), title_size);
}

// $0143（CGB flag）を取得
CartridgeTarget CartridgeHeader::GetCartridgeTarget(
    const std::vector<std::uint8_t>& rom) {
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

// $0147（Cartridge type）を取得
CartridgeType CartridgeHeader::GetCartridgeType(
    const std::vector<std::uint8_t>& rom) {
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
std::uint32_t CartridgeHeader::GetRomSize(
    const std::vector<std::uint8_t>& rom) {
  std::uint8_t value = rom[0x148];
  if (0 <= value && value <= 8) {
    return (32 << value);
  } else {
    Error("Invalid cartridge header at $0148: %d.", static_cast<int>(value));
  }
}

// $0149（RAM size）を取得
std::uint32_t CartridgeHeader::GetRamSize(
    const std::vector<std::uint8_t>& rom) {
  std::uint8_t value = rom[0x149];
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

std::string CartridgeHeader::GetCartridgeTargetString(CartridgeTarget target) {
  switch (target) {
    case CartridgeTarget::kGb:
      return "GB";
    case CartridgeTarget::kGbAndGbc:
      return "GB/GBC";
    case CartridgeTarget::kGbc:
      return "GBC";
  }
}

std::string CartridgeHeader::GetCartridgeTypeString(CartridgeType type) {
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

void CartridgeHeader::Print() {
  std::cout << "=== Cartridge Infomation ===\n";
  std::cout << "title: " << title << "\n";
  std::cout << "target: " << GetCartridgeTargetString(target) << "\n";
  std::cout << "type: " << GetCartridgeTypeString(type) << "\n";
  std::cout << "rom_size: " << rom_size << " KiB\n";
  std::cout << "ram_size: " << ram_size << " KiB\n";
  std::cout << "============================\n";
}

}  // namespace gbemu
