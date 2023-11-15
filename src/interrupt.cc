#include "interrupt.h"

#include <cstdint>

#include "utils.h"

namespace gbemu {

std::uint16_t Interrupt::GetInterruptHandlerAddress(
    Interrupt::InterruptSource source) {
  switch (source) {
    case kVblank:
      return 0x40;
    case kStat:
      return 0x48;
    case kTimer:
      return 0x50;
    case kSerial:
      return 0x58;
    case kJoypad:
      return 0x60;
    default:
      UNREACHABLE("Unknown interrupt source: %d", source);
  }
}

Interrupt::InterruptSource Interrupt::GetRequestedInterrupt() {
  std::uint8_t allowed = if_ & ie_;
  int min_set_bit_pos = __builtin_ffs(allowed) - 1;
  InterruptSource source = static_cast<InterruptSource>(min_set_bit_pos);
  ASSERT(source < kInterruptSourceNum, "Invalid interrupt source: %d", source);
  return source;
}

}  // namespace gbemu
