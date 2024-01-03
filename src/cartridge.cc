#include "cartridge.h"

#include <cstdint>
#include <vector>

#include "cartridge_header.h"
#include "mbc.h"
#include "utils.h"

namespace gbemu {

Cartridge::Cartridge(std::vector<std::uint8_t>& rom,
                     std::vector<std::uint8_t>* ram)
    : header_(), rom_(rom), ram_(ram), mbc_() {
  // カートリッジヘッダのパース
  header_.Parse(rom_);

  // ROMサイズのチェック
  auto rom_size_in_header = header_.rom_size() * 1024;
  if (rom_.size() != rom_size_in_header) {
    Error("Actual ROM size is not consistent with the header.");
  }

  // RAMの初期化
  ASSERT(ram_ != nullptr, "Invalid argument.");
  auto ram_size_in_header = header_.ram_size() * 1024;
  if (ram_->size() != 0 && ram_->size() != ram_size_in_header) {
    WARN("Failed to load the save data: create a new data.");
  }
  if (ram_->size() != ram_size_in_header) {
    *ram_ = std::vector<std::uint8_t>(ram_size_in_header, 0);
  }

  // MBCの作成
  mbc_ = Mbc::Create(header_.type(), rom_, *ram_);
}

std::uint8_t Cartridge::Read8(std::uint16_t address) const {
  return mbc_->Read8(address);
}

void Cartridge::Write8(std::uint16_t address, std::uint8_t value) {
  mbc_->Write8(address, value);
}

}  // namespace gbemu
