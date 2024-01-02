#ifndef GBEMU_JOYPAD_H_
#define GBEMU_JOYPAD_H_

#include <cstdint>

#include "interrupt.h"

namespace gbemu {

class Joypad {
 public:
  Joypad(Interrupt& interrupt) : interrupt_(interrupt) {}

  enum class Key { kA, kB, kSelect, kStart, kRight, kLeft, kUp, kDown };
  enum class KeyKind { kAction, kDirection };

  // P1の値を得る
  std::uint8_t get_p1() const { return p1_; }

  // P1に値を設定する。
  // アクション/方向キー選択ビットのみ上書きする。
  void set_p1(std::uint8_t value);

  // キーを押す。すでに押していたら何も起こらない。
  void PressKey(Key key);

  // キーを離す。すでに離していたら何も起こらない。
  void ReleaseKey(Key key);

 private:
  static constexpr unsigned kInputARightBitMask = 1 << 0;
  static constexpr unsigned kInputBLeftBitMask = 1 << 1;
  static constexpr unsigned kInputSelectUpBitMask = 1 << 2;
  static constexpr unsigned kInputStartDownBitMask = 1 << 3;
  static constexpr unsigned kSelectDirectionKeysBitMask = 1 << 4;
  static constexpr unsigned kSelectActionKeysBitMask = 1 << 5;
  static constexpr KeyKind key_kinds_[2] = {KeyKind::kAction,
                                            KeyKind::kDirection};
  static constexpr Key action_keys_[4] = {Key::kA, Key::kB, Key::kSelect,
                                          Key::kStart};
  static constexpr Key direction_keys_[4] = {Key::kRight, Key::kLeft, Key::kUp,
                                             Key::kDown};
  static constexpr auto kNumOfKeyKinds = sizeof(key_kinds_) / sizeof(KeyKind);
  static constexpr auto kNumOfActionKeys = sizeof(action_keys_) / sizeof(Key);
  static constexpr auto kNumOfDirectionKeys =
      sizeof(direction_keys_) / sizeof(Key);

  // キーに対応するビット位置に1を立てた値を得る
  static unsigned GetKeyBitMask(Key key);

  // キーの種別を得る
  static KeyKind GetKeyKind(Key key);

  // 与えられた種類のキーが入力として選択されているか調べる
  bool IsKeyKindSelected(KeyKind kind) const;

  // キーに対応するP1のビットが立っていたらtrue、立っていなければfalseを返す
  bool GetKeyBit(Key key) const;

  // キーに対応するP1のビットを立てる
  void SetKeyBit(Key key);

  // キーに対応するP1のビットを下ろす
  void ResetKeyBit(Key key);

  // キーが押されているかどうかを記憶するメンバ変数を取得する
  bool& GetKeyPressedFlag(Key key);

  bool is_a_key_pressed_{false};
  bool is_b_key_pressed_{false};
  bool is_select_key_pressed_{false};
  bool is_start_key_pressed_{false};
  bool is_right_key_pressed_{false};
  bool is_left_key_pressed_{false};
  bool is_up_key_pressed_{false};
  bool is_down_key_pressed_{false};
  std::uint8_t p1_{0xFF};
  Interrupt& interrupt_;
};

}  // namespace gbemu

#endif  // GBEMU_JOYPAD_H_
