#ifndef GBEMU_MBC_H_
#define GBEMU_MBC_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "cartridge_header.h"

namespace gbemu {

using std::uint16_t;
using std::uint8_t;

class Mbc {
 public:
  virtual uint8_t Read8(uint16_t address,
                        const std::vector<uint8_t>& rom_data) = 0;
  virtual void Write8(uint16_t address, uint8_t value) = 0;
  static std::shared_ptr<Mbc> Create(CartridgeHeader::CartridgeType type);
  virtual ~Mbc() = default;
};

}  // namespace gbemu

#endif  // GBEMU_MBC_H_
