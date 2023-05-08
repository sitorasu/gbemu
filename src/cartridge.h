#ifndef GBEMU_CARTRIDGE_H_
#define GBEMU_CARTRIDGE_H_

#include <cstdint>
#include <utility>
#include <vector>

#include "cartridge_header.h"

namespace gbemu {

class Cartridge {
  using uint8_t = std::uint8_t;
  using uint16_t = std::uint16_t;
  using uint32_t = std::uint32_t;

 public:
  // 初期化順序（header_が先）に依存した処理のため注意
  Cartridge(std::vector<uint8_t>&& data)
      : header_(data), data_(std::move(data)) {}
  virtual uint8_t Read8(uint16_t address) = 0;
  virtual void Write8(uint16_t address, uint8_t value) = 0;

 private:
  CartridgeHeader header_;
  std::vector<uint8_t> data_;
};

class NoMbcCartridge : public Cartridge {};

}  // namespace gbemu

#endif  // GBEMU_CARTRIDGE_H_
