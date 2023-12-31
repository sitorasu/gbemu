#include "cartridge.h"

#include <cstdint>
#include <vector>

#include "cartridge_header.h"
#include "mbc.h"
#include "utils.h"

namespace gbemu {

Cartridge::Cartridge(std::vector<std::uint8_t>& rom)
    : header_(), rom_(rom), ram_(), mbc_() {
  header_.Parse(rom_);
  if (header_.rom_size() * 1024 != rom_.size()) {
    Error("Actual ROM size is not consistent with the header.");
  }
  ram_.resize(header_.ram_size() * 1024);
  mbc_ = Mbc::Create(header_.type(), rom_, ram_);
}

std::uint8_t Cartridge::Read8(std::uint16_t address) const {
  return mbc_->Read8(address);
}

void Cartridge::Write8(std::uint16_t address, std::uint8_t value) {
  mbc_->Write8(address, value);
}

}  // namespace gbemu
