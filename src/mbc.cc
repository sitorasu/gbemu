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
  uint8_t Read8(uint16_t address) override { return rom_.at(address); }
  void Write8(uint16_t address, uint8_t value) override {
    // 何もしない
  }
  RomOnly(const std::vector<uint8_t>& rom, std::vector<uint8_t>& ram)
      : Mbc(rom, ram) {}
  ~RomOnly() override = default;
};

class Mbc1 : public Mbc {
 public:
  uint8_t Read8(uint16_t address) override { assert(false); }
  void Write8(uint16_t address, uint8_t value) override { assert(false); }
  Mbc1(const std::vector<uint8_t>& rom, std::vector<uint8_t>& ram)
      : Mbc(rom, ram) {}
  ~Mbc1() override = default;
};

std::shared_ptr<Mbc> Mbc::Create(CartridgeHeader::CartridgeType type,
                                 const std::vector<uint8_t>& rom,
                                 std::vector<uint8_t>& ram) {
  switch (type) {
    case CartridgeHeader::CartridgeType::kRomOnly:
      return std::make_shared<RomOnly>(rom, ram);
    case CartridgeHeader::CartridgeType::kMbc1:
    case CartridgeHeader::CartridgeType::kMbc1Ram:
    case CartridgeHeader::CartridgeType::kMbc1RamBattery:
      return std::make_shared<Mbc1>(rom, ram);
    default:
      assert(false);
  }
}

}  // namespace gbemu
