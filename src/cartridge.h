#ifndef GBEMU_CARTRIDGE_H_
#define GBEMU_CARTRIDGE_H_

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "cartridge_header.h"

namespace gbemu {

class Cartridge {
 private:
  using uint8_t = std::uint8_t;
  using uint16_t = std::uint16_t;
  using uint32_t = std::uint32_t;

  class Mbc {
   public:
    virtual uint8_t Read8(uint16_t address,
                          const std::vector<uint8_t>& rom_data) = 0;
    virtual void Write8(uint16_t address, uint8_t value) = 0;
  };

 public:
  // 初期化順序（header_が先）に依存した処理のため注意。
  Cartridge(std::vector<uint8_t>&& rom_data)
      : header_(CartridgeHeader::Create(rom_data)),
        rom_data_(std::move(rom_data)) {
    if (header_.rom_size * 1024 != rom_data.size()) {
      std::cerr << "rom_size_ error\n";
      std::exit(0);
    }
  }
  uint8_t Read8(uint16_t address) { return mbc_->Read8(address, rom_data_); }
  void Write8(uint16_t address, uint8_t value) { mbc_->Write8(address, value); }

 private:
  const CartridgeHeader header_;
  const std::vector<uint8_t> rom_data_;
  const std::unique_ptr<Mbc> mbc_;
};

}  // namespace gbemu

#endif  // GBEMU_CARTRIDGE_H_
