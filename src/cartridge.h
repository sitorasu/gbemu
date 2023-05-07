#ifndef GBEMU_CARTRIDGE_H_
#define GBEMU_CARTRIDGE_H_

#include <cstdint>
#include <vector>

#include "cartridge_header.h"

namespace gbemu {

class Cartridge {
 public:
  virtual uint8_t read8(std::uint16_t address) = 0;
  virtual void write8(std::uint16_t address, std::uint8_t value) = 0;

 private:
  CartridgeHeader header;
  std::vector<std::uint8_t> data;
};

}  // namespace gbemu

#endif  // GBEMU_CARTRIDGE_H_
