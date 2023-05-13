#include "mbc.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include "cartridge_header.h"

using std::uint16_t;
using std::uint8_t;

namespace gbemu {

class RomOnly : public Mbc {
 public:
  uint8_t Read8(uint16_t address,
                const std::vector<uint8_t>& rom_data) override {
    return rom_data.at(address);
  }
  void Write8(uint16_t address, uint8_t value) override {
    // 何もしない
  }
  ~RomOnly() override = default;
};

class Mbc1 : public Mbc {
 public:
  uint8_t Read8(uint16_t address,
                const std::vector<uint8_t>& rom_data) override {
    assert(false);
  }
  void Write8(uint16_t address, uint8_t value) override { assert(false); }
  ~Mbc1() override = default;
};

std::shared_ptr<Mbc> Mbc::Create(CartridgeHeader::CartridgeType type) {
  switch (type) {
    case CartridgeHeader::CartridgeType::kRomOnly:
      return std::make_shared<RomOnly>();
    case CartridgeHeader::CartridgeType::kMbc1:
    case CartridgeHeader::CartridgeType::kMbc1Ram:
    case CartridgeHeader::CartridgeType::kMbc1RamBattery:
      return std::make_shared<Mbc1>();
    default:
      assert(false);
  }
}

}  // namespace gbemu
