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

// カートリッジを表すクラス。Mbcを介して操作する。
// Example:
//    std::vector<std::uint8_t> rom = LoadRom();
//    Cartridge cartridge(std::move(rom));
//    std::uint16_t address1 = 0x3000;
//    std::uint8_t result = cartridge.mbc().Read8(address1);
//    std::uint16_t address2 = 0xC000;
//    cartridge.mbc().Write8(address2, 0xFF);
class Cartridge {
 public:
  // `rom`の内容をもとにインスタンスを生成する。
  // `rom`の内容が不正な場合はプログラムを終了する。
  // 初期化順序（header_, rom_, ram_, mbc_の順）に依存した処理のため注意。
  Cartridge(std::vector<std::uint8_t>&& rom)
      : header_(CartridgeHeader::Create(rom)),
        rom_(std::move(rom)),
        ram_(header_.ram_size * 1024),
        mbc_(Mbc::Create(header_.type, rom_, ram_)) {
    if (header_.rom_size * 1024 != rom_.size()) {
      Error("ROM size is not consistent with the header.");
    }
  }
  Mbc& mbc() { return *mbc_; }

 private:
  const CartridgeHeader header_;
  const std::vector<std::uint8_t> rom_;
  std::vector<std::uint8_t> ram_;
  std::unique_ptr<Mbc> mbc_;
};

}  // namespace gbemu

#endif  // GBEMU_CARTRIDGE_H_
