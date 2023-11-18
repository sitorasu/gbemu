#ifndef GBEMU_INTERRUPT_H_
#define GBEMU_INTERRUPT_H_

#include <cstdint>

namespace gbemu {

// 割り込み要因の一覧。
// 列挙子の値はIEおよびIFのビット位置に対応する。
enum class InterruptSource {
  kNone = -1,
  kVblank,
  kStat,
  kTimer,
  kSerial,
  kJoypad,
  kInterruptSourceNum,
};

class Interrupt {
 public:
  Interrupt() : if_(0), ie_(0) {}

  // 割り込み要因に対応するハンドラのアドレスを得る
  static std::uint16_t GetInterruptHandlerAddress(InterruptSource source);

  // 現在対処すべき割り込み要因を返す。
  // 具体的には、IFとIEのビットが共に1になっている割り込み要因のうち、最も優先度が高いものを返す。
  // 対処すべき割り込み要因がなければkNoneを返す。
  InterruptSource GetRequestedInterrupt();

  void SetIf(std::uint8_t value) { if_ = value & 0x1F; };
  std::uint8_t GetIf() { return if_; };
  void SetIe(std::uint8_t value) { ie_ = value & 0x1F; };
  std::uint8_t GetIe() { return ie_; };
  void SetIfBit(InterruptSource source);
  void ResetIfBit(InterruptSource source);
  void SetIeBit(InterruptSource source);
  void ResetIeBit(InterruptSource source);

 private:
  std::uint8_t if_;  // Interrupt flag (IF)
  std::uint8_t ie_;  // Interrupt enable (IE)
};

}  // namespace gbemu

#endif  // GBEMU_INTERRUPT_H_
