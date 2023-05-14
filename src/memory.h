#ifndef GBEMU_MEMORY_H_
#define GBEMU_MEMORY_H_

#include <cstdint>
#include <vector>

#include "cartridge.h"

namespace gbemu {

// CPUから見たメモリを表すクラス
// メモリアクセスを各コンポーネントに振り分ける
// CPUから見える通りに扱えるというだけで、エミュレータの実装としてはCPU以外が使ってもいい
class Memory {
 public:
  Memory(Cartridge& cartridge)
      : cartridge_(cartridge), internal_ram_(kInternalRamSize) {}
  // 各コンポーネントへの参照を渡すコンストラクタがあると良さそう
  std::uint8_t Read8(std::uint16_t address);
  void Write8(std::uint16_t address, std::uint8_t value);

 private:
  // ROM、内蔵RAM、PPUなどへの参照を持っていると良さそう
  constexpr static auto kInternalRamSize = 8 * 1024;
  Cartridge& cartridge_;
  std::vector<std::uint8_t> internal_ram_;
};

}  // namespace gbemu

#endif  // GBEMU_MEMORY_H_
