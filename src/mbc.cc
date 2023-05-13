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
  RomOnly(const std::vector<uint8_t>& rom, std::vector<uint8_t>& ram)
      : Mbc(rom, ram) {}
  ~RomOnly() override = default;

 private:
  uint8_t Read8_(uint16_t address) override { return rom_.at(address); }
  void Write8_(uint16_t address, uint8_t value) override {
    // 何もしない
  }
};

class Mbc1 : public Mbc {
 public:
  Mbc1(const std::vector<uint8_t>& rom, std::vector<uint8_t>& ram)
      : Mbc(rom, ram) {}
  ~Mbc1() override = default;

 private:
  uint8_t Read8_(uint16_t address) override { assert(false); }
  void Write8_(uint16_t address, uint8_t value) override { assert(false); }
};

std::unique_ptr<Mbc> Mbc::Create(CartridgeHeader::CartridgeType type,
                                 const std::vector<uint8_t>& rom,
                                 std::vector<uint8_t>& ram) {
  switch (type) {
    case CartridgeHeader::CartridgeType::kRomOnly:
      return std::make_unique<RomOnly>(rom, ram);
    case CartridgeHeader::CartridgeType::kMbc1:
    case CartridgeHeader::CartridgeType::kMbc1Ram:
    case CartridgeHeader::CartridgeType::kMbc1RamBattery:
      return std::make_unique<Mbc1>(rom, ram);
    default:
      assert(false);
  }
}

}  // namespace gbemu
