#include "cartridge_header.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

namespace gbemu {

CartridgeHeader::CartridgeHeader(std::string title, CartridgeTarget target,
                                 CartridgeType type, std::uint32_t rom_size,
                                 std::uint32_t ram_size)
    : title(title),
      target(target),
      type(type),
      rom_size(rom_size),
      ram_size(ram_size) {}

CartridgeHeader CartridgeHeader::create(std::uint8_t* rom) {
  // $0134-0143（title, 16バイト）を取得
  std::string title(reinterpret_cast<const char*>(&rom[0x134]), 16);

  // $0143（CGB flag）を取得
  // このアドレスはtitleの末尾と被っている。意味のある値が入っている場合はtitleの末尾を削除する。
  CartridgeTarget target = [&title, rom]() {
    std::uint8_t value = rom[0x143];
    switch (value) {
      case 0x80:
        title.pop_back();
        return CartridgeTarget::kGbAndGbc;
      case 0xC0:
        title.pop_back();
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
  }();

  // $0147（Cartridge type）を取得
  CartridgeType type = [rom]() {
    std::uint8_t value = rom[0x147];
    switch (value) {
      case 0x00:
        return CartridgeType::kRomOnly;
      case 0x01:
        return CartridgeType::kMbc1;
      case 0x03:
        return CartridgeType::kMbc1RamBattery;
      default:
        std::cerr << "Cartridge type error at $0147: "
                  << static_cast<int>(value) << "\n";
        std::exit(0);
    }
  }();

  // $0148（ROM size）を取得
  std::uint32_t rom_size = [rom]() {
    std::uint8_t value = rom[0x148];
    if (0 <= value && value <= 8) {
      return (32 << value);
    } else {
      std::cerr << "Cartridge ROM size error at $0148: "
                << static_cast<int>(value) << "\n";
      std::exit(0);
    }
  }();

  // $0149（RAM size）を取得
  std::uint32_t ram_size = [rom]() {
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
        std::cerr << "Cartridge RAM size error at $0149: "
                  << static_cast<int>(value) << "\n";
        std::exit(0);
    }
  }();

  return CartridgeHeader(title, target, type, rom_size, ram_size);
}

std::string CartridgeHeader::getCartridgeTargetString(CartridgeTarget target) {
  switch (target) {
    case CartridgeTarget::kGb:
      return "GB";
    case CartridgeTarget::kGbAndGbc:
      return "GB/GBC";
    case CartridgeTarget::kGbc:
      return "GBC";
  }
}

std::string CartridgeHeader::getCartridgeTypeString(CartridgeType type) {
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

void CartridgeHeader::print() {
  std::cout << "title: " << title << "\n";
  std::cout << "target: " << getCartridgeTargetString(target) << "\n";
  std::cout << "type: " << getCartridgeTypeString(type) << "\n";
  std::cout << "rom_size: " << rom_size << " KiB\n";
  std::cout << "ram_size: " << ram_size << " KiB\n";
}

}  // namespace gbemu
