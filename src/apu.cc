#include "apu.h"

#include <cstdint>

#include "utils.h"

using namespace gbemu;

const unsigned Apu::wave_duty_table[4][8] = {{0, 0, 0, 0, 0, 0, 0, 1},
                                             {0, 0, 0, 0, 0, 0, 1, 1},
                                             {0, 0, 0, 0, 1, 1, 1, 1},
                                             {1, 1, 1, 1, 1, 1, 0, 0}};

std::uint8_t Apu::get_nr52() const {
  std::uint8_t result = 0;
  if (is_apu_enabled_) {
    result |= 1 << 7;
  }
  if (channel1_.IsEnabled()) {
    result |= 1;
  }
  if (channel2_.IsEnabled()) {
    result |= 1 << 1;
  }
  // if (channel3_.IsEnabled()) {
  //   result |= 1 << 2;
  // }
  // if (channel4_.IsEnabled()) {
  //   result |= 1 << 3;
  // }
  result |= 0x70;  // オープンビット
  return result;
}

void Apu::set_nr52(std::uint8_t value) {
  bool was_apu_enabled = is_apu_enabled_;
  is_apu_enabled_ = value >> 7;
  if (was_apu_enabled && !is_apu_enabled_) {
    ResetApu();
  }
}

void Apu::Envelope::Step() {
  // Periodが0の場合Envelopeは無効
  if (period_ == 0) {
    return;
  }

  // ASSERT(period_timer_ > 0, "Period Timer should be reloaded.");
  if (period_timer_ > 0) {
    period_timer_--;
  }

  if (period_timer_ == 0) {
    period_timer_ = period_;

    if ((current_volume_ < 0xF && is_upward_) ||
        (current_volume_ > 0 && !is_upward_)) {
      int adjustment = is_upward_ ? 1 : -1;
      current_volume_ += adjustment;
    }
  }
}

void Apu::Envelope::Trigger() {
  current_volume_ = initial_volume_;
  period_timer_ = period_;
}

void Apu::Sweep::SetPeriod(unsigned value) {
  unsigned old_period = period_;
  period_ = value & 0x07;
  if (period_ == 0) {
    timer_ = 0;
  } else if (old_period == 0) {
    timer_ = period_;
  }
}

void Apu::Sweep::Trigger(unsigned initial_frequency) {
  current_frequency_ = initial_frequency;
  timer_ = period_;
}

unsigned Apu::Sweep::CalculateNewFrequency() const {
  unsigned adjustment = current_frequency_ >> shift_amount_;
  if (is_upward_) {
    return current_frequency_ + adjustment;
  } else {
    return current_frequency_ - adjustment;
  }
}

bool Apu::Sweep::Step() {
  // Timerが0でここに到達するのは、Periodに0が設定されたことによってTimerが0になったとき
  if (timer_ == 0) {
    return false;
  }

  timer_--;

  if (timer_ == 0) {
    timer_ = period_;
    current_frequency_ = CalculateNewFrequency();
    ASSERT(
        current_frequency_ < 2048,
        "If the new frequency overflows, sweep must be stopped immediately.");
    if (CalculateNewFrequency() > 2047) {
      return true;
    }
  }

  return false;
}

std::uint8_t Apu::PulseChannel::GetNrX0() const {
  std::uint8_t result = 0;
  result |= sweep_.GetShiftAmount();
  if (sweep_.IsUpward()) {
    result |= 1 << 3;
  }
  result |= sweep_.GetPeriod() << 4;
  result |= 1 << 7;  // オープンのビット
  return result;
}

void Apu::PulseChannel::SetNrX0(std::uint8_t value) {
  unsigned shift_amount = value & 0x7;
  value >>= 3;
  bool is_upward = value & 1;
  value >>= 1;
  unsigned period = value;

  sweep_.SetShiftAmount(shift_amount);
  sweep_.SetDirection(is_upward);
  sweep_.SetPeriod(period);
}

std::uint8_t Apu::PulseChannel::GetNrX1() const {
  std::uint8_t result = 0;
  result |= length_timer_.Get();
  result |= wave_duty_pattern_ << 6;
  result |= 0x3F;  // オープンのビット
  return result;
}

void Apu::PulseChannel::SetNrX1(std::uint8_t value) {
  length_timer_.Set(value & 0x3F);
  value >>= 6;
  wave_duty_pattern_ = value;
}

std::uint8_t Apu::PulseChannel::GetNrX2() const {
  std::uint8_t result = 0;
  result |= envelope_.GetPeriod();
  if (envelope_.IsUpward()) {
    result |= 1 << 3;
  }
  result |= envelope_.GetInitialVolume() << 4;
  return result;
}

void Apu::PulseChannel::SetNrX2(std::uint8_t value) {
  is_dac_enabled_ = (value & 0xF8) != 0;
  is_enabled_ &= is_dac_enabled_;

  unsigned period = value & 0x7;
  value >>= 3;
  bool is_upward = value & 1;
  value >>= 1;
  unsigned initial_volume = value;

  envelope_.SetPeriod(period);
  envelope_.SetDirection(is_upward);
  envelope_.SetInitialVolume(initial_volume);
}

void Apu::PulseChannel::SetNrX3(std::uint8_t value) {
  freqency_timer_.SetLower8BitOfFrequency(value);
}

std::uint8_t Apu::PulseChannel::GetNrX4() const {
  std::uint8_t result = length_timer_.IsEnabled() ? 1 << 6 : 0;
  result |= 0xBF;  // オープンのビット
  return result;
}

void Apu::PulseChannel::SetNrX4(std::uint8_t value) {
  freqency_timer_.SetUpper3BitOfFrequency(value & 0x7);
  value >>= 6;
  if ((value & 1) != 0) {
    length_timer_.TurnOn();
  } else {
    length_timer_.TurnOff();
  }
  value >>= 1;
  if (value == 0) {
    is_enabled_ = false;
  } else if (!is_enabled_ && is_dac_enabled_) {
    Trigger();
    is_enabled_ = true;
  }
}

void Apu::PulseChannel::Trigger() {
  unsigned frequency = freqency_timer_.GetFrequency();
  sweep_.Trigger(frequency);

  envelope_.Trigger();
}

void Apu::PulseChannel::StepSweep() {
  bool channel_off_signal = sweep_.Step();
  unsigned frequency = sweep_.GetFrequency();
  freqency_timer_.SetFrequency(frequency);
  if (channel_off_signal) {
    is_enabled_ = false;
  }
}

void Apu::PulseChannel::StepLengthTimer() {
  bool channel_off_signal = length_timer_.Step();
  if (channel_off_signal) {
    is_enabled_ = false;
  }
}

void Apu::PulseChannel::StepEnvelope() { envelope_.Step(); }

void Apu::PulseChannel::StepFrequencyTimer() {
  bool increment = freqency_timer_.Step();
  if (increment) {
    wave_duty_position_ = (wave_duty_position_ + 1) % 8;
  }
}

// NR52のビット7を0にしたときの処理
// Wave RAMはクリアされない
void Apu::ResetApu() {
  channel1_ = PulseChannel();
  channel2_ = PulseChannel();
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
}

void Apu::Step() {
  if (!is_apu_enabled_) {
    return;
  }

  channel1_.StepFrequencyTimer();
  channel2_.StepFrequencyTimer();
  bool clocked = frame_sequencer_.Step();
  if (clocked) {
    unsigned pos = frame_sequencer_.GetPos();
    switch (pos) {
      // Step   Length Ctr  Vol Env     Sweep
      // ---------------------------------------
      // 0      Clock       -           -
      // 1      -           -           -
      // 2      Clock       -           Clock
      // 3      -           -           -
      // 4      Clock       -           -
      // 5      -           -           -
      // 6      Clock       -           Clock
      // 7      -           Clock       -
      case 0:
        channel1_.StepLengthTimer();
        channel2_.StepLengthTimer();
        break;
      case 1:
        break;
      case 2:
        channel1_.StepLengthTimer();
        channel1_.StepSweep();
        channel2_.StepLengthTimer();
        channel2_.StepSweep();
        break;
      case 3:
        break;
      case 4:
        channel1_.StepLengthTimer();
        channel2_.StepLengthTimer();
        break;
      case 5:
        break;
      case 6:
        channel1_.StepLengthTimer();
        channel1_.StepSweep();
        channel2_.StepLengthTimer();
        channel2_.StepSweep();
        break;
      case 7:
        channel1_.StepEnvelope();
        channel2_.StepEnvelope();
        break;
      default:
        UNREACHABLE("Invalid Position!");
    }
  }

  // TODO: 続きを書く（95クロックごとにバッファにサンプルを供給する）
  static int sample_counter = 95;
  if (--sample_counter == 0) {
    sample_counter = 95;
  }
}

void Apu::Run(unsigned tcycles) {
  for (unsigned i = 0; i < tcycles; i++) {
    Step();
  }
}
