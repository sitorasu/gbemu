#include "apu.h"

#include <cstdint>

using namespace gbemu;

void Apu::set_nr52(std::uint8_t value) {
  bool was_apu_enabled = nr52_.IsApuEnabled();
  nr52_.set(value);
  bool is_apu_enabled = nr52_.IsApuEnabled();
  if (was_apu_enabled && !is_apu_enabled) {
    ResetApu();
  }
}

void Apu::ResetApu() {
  nr10_ = 0;
  nr11_ = 0;
  nr12_ = 0;
  nr13_ = 0;
  nr14_ = 0;
  nr21_ = 0;
  nr22_ = 0;
  nr23_ = 0;
  nr24_ = 0;
  nr30_ = 0;
  nr31_ = 0;
  nr32_ = 0;
  nr33_ = 0;
  nr34_ = 0;
  nr41_ = 0;
  nr42_ = 0;
  nr43_ = 0;
  nr44_ = 0;
  nr50_.set(0);
  nr51_.set(0);
  nr52_.set(0);
  wave_ram_.fill(0);
}
