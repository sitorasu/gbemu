#ifndef GBEMU_TIMER_H_
#define GBEMU_TIMER_H_

#include <cstdint>

#include "interrupt.h"

namespace gbemu {

class Timer {
 public:
  Timer(Interrupt& interrupt) : interrupt_(interrupt) {}

  std::uint8_t div() const { return counter_ >> 8; }
  std::uint8_t tima() const { return tima_; }
  std::uint8_t tma() const { return tma_; }
  std::uint8_t tac() const { return tac_; }

  void reset_div() { counter_ = 0; }
  void set_tima(std::uint8_t value) { tima_ = value; }
  void set_tma(std::uint8_t value) { tma_ = value; }
  void set_tac(std::uint8_t value) { tac_ = value; }

  // 指定したクロック数だけ状態を進める。
  void Run(unsigned tcycle);

 private:
  // 1クロックだけ状態を進める。
  void Step();

  // TACで設定したTIMAのインクリメントの周波数を
  // CPUクロックの周波数/(2^n) と表したときのnを返す。
  unsigned GetTimaDivisorInLog2() const;

  // Timaのカウントが有効であるか否かを返す。
  bool IsTimaEnable() const { return tac_ & 4; }

  Interrupt& interrupt_;

  std::uint8_t tima_{};
  std::uint8_t tma_{};
  std::uint8_t tac_{};

  // CPUクロックと同じ周波数でインクリメントするカウンタ。
  // 上位8ビットがDIVレジスタ。
  std::uint16_t counter_{};
};

}  // namespace gbemu

#endif  // GBEMU_TIMER_H_
