#ifndef GBEMU_SERIAL_H_
#define GBEMU_SERIAL_H_

#include <cstdint>
#include <iostream>

#include "command_line.h"

namespace gbemu {

// シリアル通信機能を表すクラス。
// 送信データを標準出力するだけの実装になっている。
class Serial {
 public:
  std::uint8_t sb() const { return sb_; }
  std::uint8_t sc() const { return sc_; }
  void set_sb(std::uint8_t value) { sb_ = value; }
  void set_sc(std::uint8_t value) {
    sc_ = value;
    sc_ &= ~(0b10000001U);
    // --debugフラグがない場合に限り、シリアル出力を標準出力に接続
    // --debugフラグがあるときは接続しない（デバッグ出力と混ざってぐちゃぐちゃになるので）
    if (!options.debug() && value == 0x81) {
      std::cout << static_cast<unsigned char>(sb_) << std::flush;
    }
  }

 private:
  std::uint8_t sb_{};
  std::uint8_t sc_{};
};

}  // namespace gbemu

#endif  // GBEMU_SERIAL_H_
