#include "joypad.h"

#include "interrupt.h"

using namespace gbemu;

void Joypad::set_p1(std::uint8_t value) {
  value &= kSelectActionKeysBitMask | kSelectDirectionKeysBitMask;
  p1_ &= ~kSelectActionKeysBitMask & ~kSelectDirectionKeysBitMask;
  p1_ |= value;

  for (unsigned i = 0; i < kNumOfKeyKinds; i++) {
    if (IsKeyKindSelected(key_kinds_[i])) {
      const Key* keys =
          key_kinds_[i] == KeyKind::kAction ? action_keys_ : direction_keys_;
      static_assert(kNumOfActionKeys == kNumOfDirectionKeys);
      for (unsigned j = 0; j < kNumOfActionKeys; j++) {
        if (GetKeyPressedFlag(keys[j])) {
          if (GetKeyBit(keys[j])) {
            ResetKeyBit(keys[j]);
            interrupt_.SetIfBit(InterruptSource::kJoypad);
          }
        } else {
          SetKeyBit(keys[j]);
        }
      }
    }
  }
}

void Joypad::PressKey(Key key) {
  KeyKind kind = GetKeyKind(key);
  bool& is_key_pressed = GetKeyPressedFlag(key);
  if (IsKeyKindSelected(kind) && GetKeyBit(key)) {
    ResetKeyBit(key);
    interrupt_.SetIfBit(InterruptSource::kJoypad);
  }
  is_key_pressed = true;
}

void Joypad::ReleaseKey(Key key) {
  KeyKind kind = GetKeyKind(key);
  bool& is_key_pressed = GetKeyPressedFlag(key);
  if (IsKeyKindSelected(kind)) {
    SetKeyBit(key);
  }
  is_key_pressed = false;
}

unsigned Joypad::GetKeyBitMask(Key key) {
  switch (key) {
    case Key::kA:
    case Key::kRight:
      return kInputARightBitMask;
    case Key::kB:
    case Key::kLeft:
      return kInputBLeftBitMask;
    case Key::kSelect:
    case Key::kUp:
      return kInputSelectUpBitMask;
    case Key::kStart:
    case Key::kDown:
      return kInputStartDownBitMask;
  }
}

Joypad::KeyKind Joypad::GetKeyKind(Key key) {
  switch (key) {
    case Key::kA:
    case Key::kB:
    case Key::kSelect:
    case Key::kStart:
      return KeyKind::kAction;
    case Key::kRight:
    case Key::kLeft:
    case Key::kUp:
    case Key::kDown:
      return KeyKind::kDirection;
  }
}

bool Joypad::IsKeyKindSelected(KeyKind kind) const {
  if (kind == KeyKind::kAction) {
    return (p1_ & kSelectActionKeysBitMask) == 0;
  } else {
    return (p1_ & kSelectDirectionKeysBitMask) == 0;
  }
}

bool Joypad::GetKeyBit(Key key) const {
  switch (key) {
    case Key::kA:
    case Key::kRight:
      return p1_ & kInputARightBitMask;
    case Key::kB:
    case Key::kLeft:
      return p1_ & kInputBLeftBitMask;
    case Key::kSelect:
    case Key::kUp:
      return p1_ & kInputSelectUpBitMask;
    case Key::kStart:
    case Key::kDown:
      return p1_ & kInputStartDownBitMask;
  }
}

void Joypad::SetKeyBit(Key key) {
  unsigned mask = GetKeyBitMask(key);
  p1_ |= mask;
}

void Joypad::ResetKeyBit(Key key) {
  unsigned mask = GetKeyBitMask(key);
  p1_ &= ~mask;
}

bool& gbemu::Joypad::GetKeyPressedFlag(Key key) {
  switch (key) {
    case Key::kA:
      return is_a_key_pressed_;
    case Key::kB:
      return is_b_key_pressed_;
    case Key::kSelect:
      return is_select_key_pressed_;
    case Key::kStart:
      return is_start_key_pressed_;
    case Key::kRight:
      return is_right_key_pressed_;
    case Key::kLeft:
      return is_left_key_pressed_;
    case Key::kUp:
      return is_up_key_pressed_;
    case Key::kDown:
      return is_down_key_pressed_;
  }
}
