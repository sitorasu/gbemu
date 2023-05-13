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
  // 初期化順序（header_, rom_, ram_, mbc_の順）に依存した処理のため注意。
  Cartridge(std::vector<uint8_t>&& rom)
      : header_(CartridgeHeader::Create(rom)),
        rom_(std::move(rom)),
        ram_(header_.ram_size),
        mbc_(Mbc::Create(header_.type, rom_, ram_)) {
    if (header_.rom_size * 1024 != rom.size()) {
      std::cerr << "rom_size_ error\n";
      std::exit(0);
    }
  }
  std::shared_ptr<Mbc> mbc() { return mbc_; }

 private:
  const CartridgeHeader header_;
  const std::vector<uint8_t> rom_;
  std::vector<uint8_t> ram_;
  const std::shared_ptr<Mbc> mbc_;
};

}  // namespace gbemu

#endif  // GBEMU_CARTRIDGE_H_
