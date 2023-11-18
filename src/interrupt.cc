#include "interrupt.h"

#include <cstdint>

#include "utils.h"

namespace gbemu {

std::uint16_t Interrupt::GetInterruptHandlerAddress(InterruptSource source) {
  switch (source) {
    case InterruptSource::kVblank:
      return 0x40;
    case InterruptSource::kStat:
      return 0x48;
    case InterruptSource::kTimer:
      return 0x50;
    case InterruptSource::kSerial:
      return 0x58;
    case InterruptSource::kJoypad:
      return 0x60;
    default:
      UNREACHABLE("Unknown interrupt source: %d", source);
  }
}

InterruptSource Interrupt::GetRequestedInterrupt() {
  std::uint8_t allowed = if_ & ie_;
  int min_set_bit_pos = __builtin_ffs(allowed) - 1;
  int interrupt_source_max =
      static_cast<int>(InterruptSource::kInterruptSourceNum);
  int interrupt_source_min = static_cast<int>(InterruptSource::kNone);
  ASSERT(min_set_bit_pos >= interrupt_source_min &&
             min_set_bit_pos < interrupt_source_max,
         "Invalid interrupt request: %d", min_set_bit_pos);
  InterruptSource source = static_cast<InterruptSource>(min_set_bit_pos);
  return source;
}

void Interrupt::SetIfBit(InterruptSource source) {
  int bit_pos = static_cast<int>(source);
  if_ |= 1 << bit_pos;
}

void Interrupt::ResetIfBit(InterruptSource source) {
  int bit_pos = static_cast<int>(source);
  if_ &= ~(1 << bit_pos);
}

void Interrupt::SetIeBit(InterruptSource source) {
  int bit_pos = static_cast<int>(source);
  ie_ |= 1 << bit_pos;
}

void Interrupt::ResetIeBit(InterruptSource source) {
  int bit_pos = static_cast<int>(source);
  ie_ &= ~(1 << bit_pos);
}

}  // namespace gbemu
