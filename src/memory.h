#ifndef GBEMU_MEMORY_H_
#define GBEMU_MEMORY_H_

#include <cstdint>

namespace gbemu {

// CPUから見たメモリを表すクラス
// メモリアクセスを各コンポーネントに振り分ける
// CPUから見える通りに扱えるというだけで、エミュレータの実装としてはCPU以外が使ってもいい
class Memory {
  // ROM、内蔵RAM、PPUなどへの参照を持っていると良さそう
  std::uint8_t internal_ram[8 * 1024];

 public:
  explicit Memory();
  // 各コンポーネントへの参照を渡すコンストラクタがあると良さそう
  std::uint8_t read(std::uint16_t address);
  void write(std::uint16_t address, std::uint8_t value);
};

}  // namespace gbemu

#endif  // GBEMU_MEMORY_H_
