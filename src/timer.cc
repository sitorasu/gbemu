#include "timer.h"

#include "interrupt.h"
#include "utils.h"

namespace gbemu {

unsigned Timer::GetTimaDivisorInLog2() {
  switch (tac_ & 3) {
    case 0:
      return 10;
    case 1:
      return 4;
    case 2:
      return 6;
    case 3:
      return 8;
    default:
      UNREACHABLE("Impossible");
  }
}

void Timer::step() {
  std::uint16_t old_counter = counter_;
  counter_++;

  if (!IsTimaEnable()) {
    return;
  }

  // TIMAのインクリメントのタイミングは、TACで指定した周波数の
  // 倍の周波数のクロックの立ち下がり、つまりCPUクロックに対する
  // 分周比を2^nとしたとき、カウンタの第n-1ビットの立ち下がりである。
  unsigned n = GetTimaDivisorInLog2();
  bool old_bit = old_counter & (1 << (n - 1));
  bool new_bit = counter_ & (1 << (n - 1));
  bool falling_edge = old_bit && (!new_bit);
  if (falling_edge) {
    tima_++;

    // オーバーフローしたら割り込みフラグを立て、TMAの値をセット
    if (tima_ == 0) {
      interrupt_.SetIfBit(InterruptSource::kTimer);
      tima_ = tma_;
    }
  }
}

void Timer::step(unsigned tcycle) {
  for (unsigned i = 0; i < tcycle; i++) {
    step();
  }
}

}  // namespace gbemu
