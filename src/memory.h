#ifndef GBEMU_MEMORY_H_
#define GBEMU_MEMORY_H_

#include <cstdint>
#include <vector>

#include "cartridge.h"

namespace gbemu {

// CPUから見たメモリを表すクラス。
// メモリアクセスを各コンポーネントに振り分ける。
// CPUから見える通りに扱えるというだけで、エミュレータの実装としてはCPU以外が使ってもいい。
// Example:
//   std::vector<std::uint8_t> rom = LoadRom();
//   Cartridge cartridge(std::move(rom));
//   Memory memory(cartridge);
//
//   std::uint16_t address = 0x3000;
//   std::uint8_t result = memory.Read8(address);
class Memory {
 public:
  Memory(Cartridge& cartridge)
      : cartridge_(cartridge),
        internal_ram_(kInternalRamSize),
        h_ram_(kHRamSize) {}
  // 各コンポーネントへの参照を渡すコンストラクタがあると良さそう
  std::uint8_t Read8(std::uint16_t address);
  std::uint16_t Read16(std::uint16_t address);
  std::vector<std::uint8_t> ReadBytes(std::uint16_t address, unsigned bytes);
  void Write8(std::uint16_t address, std::uint8_t value);
  void Write16(std::uint16_t address, std::uint16_t value);

 private:
  // ROM、内蔵RAM、PPUなどへの参照を持っていると良さそう
  constexpr static auto kInternalRamSize = 8 * 1024;
  constexpr static auto kHRamSize = 127;
  Cartridge& cartridge_;
  std::vector<std::uint8_t> internal_ram_;
  std::vector<std::uint8_t> h_ram_;
};

}  // namespace gbemu

#endif  // GBEMU_MEMORY_H_
