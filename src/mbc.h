#ifndef GBEMU_MBC_H_
#define GBEMU_MBC_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "cartridge_header.h"
#include "utils.h"

namespace gbemu {

// MBCのインターフェース。
class Mbc {
 public:
  Mbc(const std::vector<std::uint8_t>& rom, std::vector<std::uint8_t>& ram)
      : rom_(rom), ram_(ram) {}
  // このクラスは継承されるためデストラクタは仮想関数にする。
  virtual ~Mbc() = default;
  // CPUによる`address`からの読み出し要求に対処する。
  virtual std::uint8_t Read8(std::uint16_t address) const = 0;
  // CPUによる`address`への書き込み要求に対処する。
  virtual void Write8(std::uint16_t address, std::uint8_t value) = 0;
  // `type`が表すMBCの種類に対応するMbcの派生クラスのインスタンスを生成する。
  static std::unique_ptr<Mbc> Create(CartridgeType type,
                                     const std::vector<std::uint8_t>& rom,
                                     std::vector<std::uint8_t>& ram);

 protected:
  // このMBCが読み出すROM。
  const std::vector<std::uint8_t>& rom_;
  // このMBCが読み書きする(External)RAM。
  std::vector<std::uint8_t>& ram_;
};

}  // namespace gbemu

#endif  // GBEMU_MBC_H_
