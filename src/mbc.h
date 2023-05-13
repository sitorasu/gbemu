#ifndef GBEMU_MBC_H_
#define GBEMU_MBC_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "cartridge_header.h"

namespace gbemu {

class Mbc {
 protected:
  using uint16_t = std::uint16_t;
  using uint8_t = std::uint8_t;

 public:
  virtual uint8_t Read8(uint16_t address) = 0;
  virtual void Write8(uint16_t address, uint8_t value) = 0;
  static std::shared_ptr<Mbc> Create(CartridgeHeader::CartridgeType type,
                                     const std::vector<uint8_t>& rom,
                                     std::vector<uint8_t>& ram);
  Mbc(const std::vector<uint8_t>& rom, std::vector<uint8_t>& ram)
      : rom_(rom), ram_(ram) {}
  virtual ~Mbc() = default;

 protected:
  const std::vector<uint8_t>& rom_;
  std::vector<uint8_t>& ram_;
};

}  // namespace gbemu

#endif  // GBEMU_MBC_H_
