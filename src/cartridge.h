#ifndef GBEMU_CARTRIDGE_H_
#define GBEMU_CARTRIDGE_H_

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "cartridge_header.h"
#include "mbc.h"

namespace gbemu {

class Cartridge {
 private:
  using uint8_t = std::uint8_t;
  using uint16_t = std::uint16_t;
  using uint32_t = std::uint32_t;

 public:
  // 初期化順序（header_, mbc_, rom_dataの順）に依存した処理のため注意。
  Cartridge(std::vector<uint8_t>&& rom_data)
      : header_(CartridgeHeader::Create(rom_data)),
        mbc_(Mbc::Create(header_.type)),
        rom_data_(std::move(rom_data)),
        ram_data_(header_.ram_size) {
    if (header_.rom_size * 1024 != rom_data.size()) {
      std::cerr << "rom_size_ error\n";
      std::exit(0);
    }
  }
  std::shared_ptr<Mbc> mbc() { return mbc_; }

 private:
  const CartridgeHeader header_;
  const std::shared_ptr<Mbc> mbc_;
  const std::vector<uint8_t> rom_data_;
  std::vector<uint8_t> ram_data_;
};

}  // namespace gbemu

#endif  // GBEMU_CARTRIDGE_H_
