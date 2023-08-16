#ifndef GBEMU_MBC_H_
#define GBEMU_MBC_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "cartridge_header.h"
#include "utils.h"

namespace gbemu {

// MBCのインターフェース。
// Example:
//   std::vector<std::uint8_t> rom = LoadRom();
//   CartridgeHeader header = CartridgeHeader::Create(rom);
//   std::vector<std::uint8_t> ram(header.ram_size * 1024);
//   CartridgeHeader::CartridgeType type = header.type;
//   std::unique_ptr<Mbc> mbc = Mbc::Create(type, rom, ram);
//   std::uint16_t address = 0x7000;
//   std::uint8_t result = mbc.Read8(address);
class Mbc {
 public:
  // CPUによる`address`からの読み出し要求に対処する。
  virtual std::uint8_t Read8(std::uint16_t address) = 0;
  // CPUによる`address`への書き込み要求に対処する。
  virtual void Write8(std::uint16_t address, std::uint8_t value) = 0;
  // `type`が表すMBCの種類に対応するMbcの派生クラスのインスタンスを生成する。
  static std::unique_ptr<Mbc> Create(CartridgeHeader::CartridgeType type,
                                     const std::vector<std::uint8_t>& rom,
                                     std::vector<std::uint8_t>& ram);
  Mbc(const std::vector<std::uint8_t>& rom, std::vector<std::uint8_t>& ram)
      : rom_(rom), ram_(ram) {}
  virtual ~Mbc() = default;

 protected:
  // このMBCが読み出すROM。
  const std::vector<std::uint8_t>& rom_;
  // このMBCが読み書きするRAM。
  std::vector<std::uint8_t>& ram_;
};

}  // namespace gbemu

#endif  // GBEMU_MBC_H_
