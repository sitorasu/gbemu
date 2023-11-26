#ifndef GBEMU_CARTRIDGE_H_
#define GBEMU_CARTRIDGE_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "cartridge_header.h"
#include "mbc.h"

namespace gbemu {

// カートリッジを表すクラス。
class Cartridge {
 public:
  // ROMの内容をもとにインスタンスを生成する。
  // ROMの内容が不正な場合はプログラムを終了する。
  Cartridge(std::vector<std::uint8_t>&& rom);

  // アドレスに応じてROMまたは(External)RAMから1バイトの値を読み出す。
  // 範囲外へのアクセスはエラーとしプログラムを終了する。
  std::uint8_t Read8(std::uint16_t address) const;
  // アドレスに応じてROMまたは(External)RAMに1バイトの値を書き込む。
  // ROMへの書き込みは実際にはMBCの操作となる。
  // 範囲外へのアクセスはエラーとしプログラムを終了する。
  void Write8(std::uint16_t address, std::uint8_t value);

 private:
  CartridgeHeader header_;
  std::vector<std::uint8_t> rom_;
  std::vector<std::uint8_t> ram_;
  std::unique_ptr<Mbc> mbc_;
};

}  // namespace gbemu

#endif  // GBEMU_CARTRIDGE_H_
