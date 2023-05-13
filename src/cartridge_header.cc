#include "cartridge_header.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace gbemu {

using CartridgeTarget = CartridgeHeader::CartridgeTarget;
using CartridgeType = CartridgeHeader::CartridgeType;
using uint8_t = std::uint8_t;
using uint32_t = std::uint32_t;

// ROMの$0134-0143（title, 16バイト）を取得
std::string CartridgeHeader::GetTitle(const std::vector<uint8_t>& rom_data) {
  auto title_size =
      GetCartridgeTarget(rom_data) == CartridgeTarget::kGb ? 16 : 15;
  return std::string(reinterpret_cast<const char*>(&rom_data[0x134]),
                     title_size);
}

// $0143（CGB flag）を取得
CartridgeTarget CartridgeHeader::GetCartridgeTarget(
    const std::vector<uint8_t>& rom_data) {
  uint8_t value = rom_data[0x143];
  switch (value) {
    case 0x80:
      return CartridgeTarget::kGbAndGbc;
    case 0xC0:
      return CartridgeTarget::kGbc;
    default:
      if (isprint(value) || (value == 0x00)) {
        return CartridgeTarget::kGb;
      } else {
        std::cerr << "Cartridge target error at $0143: "
                  << static_cast<int>(value) << "\n";
        std::exit(0);
      }
  }
}

// $0147（Cartridge type）を取得
CartridgeType CartridgeHeader::GetCartridgeType(
    const std::vector<uint8_t>& rom_data) {
  uint8_t value = rom_data[0x147];
  switch (value) {
    case 0x00:
      return CartridgeType::kRomOnly;
    case 0x01:
      return CartridgeType::kMbc1;
    case 0x03:
      return CartridgeType::kMbc1RamBattery;
    default:
      std::cerr << "Cartridge type error at $0147: " << static_cast<int>(value)
                << "\n";
      std::exit(0);
  }
}

// $0148（ROM size）を取得
uint32_t CartridgeHeader::GetRomSize(const std::vector<uint8_t>& rom_data) {
  uint8_t value = rom_data[0x148];
  if (0 <= value && value <= 8) {
    return (32 << value);
  } else {
    std::cerr << "Cartridge ROM size error at $0148: "
              << static_cast<int>(value) << "\n";
    std::exit(0);
  }
}

// $0149（RAM size）を取得
uint32_t CartridgeHeader::GetRamSize(const std::vector<uint8_t>& rom_data) {
  uint8_t value = rom_data[0x149];
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
      std::cerr << "Cartridge RAM size error at $0149: "
                << static_cast<int>(value) << "\n";
      std::exit(0);
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
  std::cout << "title: " << title << "\n";
  std::cout << "target: " << GetCartridgeTargetString(target) << "\n";
  std::cout << "type: " << GetCartridgeTypeString(type) << "\n";
  std::cout << "rom_size: " << rom_size << " KiB\n";
  std::cout << "ram_size: " << ram_size << " KiB\n";
}

}  // namespace gbemu
