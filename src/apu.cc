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
  if (channel3_.IsEnabled()) {
    result |= 1 << 2;
  }
  if (channel4_.IsEnabled()) {
    result |= 1 << 3;
  }
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
  ASSERT(value < 8, "Invalud sweep period is set!");
  unsigned old_period = period_;
  period_ = value;
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
  if (is_decrementing_) {
    return current_frequency_ - adjustment;
  } else {
    return current_frequency_ + adjustment;
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
  unsigned period = value & 0b111;

  sweep_.SetShiftAmount(shift_amount);
  sweep_.SetDirection(is_upward);
  sweep_.SetPeriod(period);
}

std::uint8_t Apu::PulseChannel::GetNrX1() const {
  std::uint8_t result = 0;
  result |= wave_duty_pattern_ << 6;
  result |= 0x3F;  // オープンのビット
  return result;
}

void Apu::PulseChannel::SetNrX1(std::uint8_t value) {
  length_timer_.InitTimer(value & 0x3F);
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
  frequency_timer_.SetLower8BitOfFrequency(value);
}

std::uint8_t Apu::PulseChannel::GetNrX4() const {
  std::uint8_t result = length_timer_.IsEnabled() ? 1 << 6 : 0;
  result |= 0xBF;  // オープンのビット
  return result;
}

void Apu::PulseChannel::SetNrX4(std::uint8_t value) {
  frequency_timer_.SetUpper3BitOfFrequency(value & 0x7);
  value >>= 6;
  if ((value & 1) != 0) {
    length_timer_.TurnOn();
  } else {
    length_timer_.TurnOff();
  }
  value >>= 1;
  if (value == 1) {
    Trigger();
    is_enabled_ = is_dac_enabled_;
  }
}

void Apu::PulseChannel::Trigger() {
  unsigned frequency = frequency_timer_.GetFrequency();
  sweep_.Trigger(frequency);
  envelope_.Trigger();
  length_timer_.Trigger();
}

void Apu::PulseChannel::StepSweep() {
  bool channel_off_signal = sweep_.Step();
  unsigned frequency = sweep_.GetFrequency();
  frequency_timer_.SetFrequency(frequency);
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
  bool increment = frequency_timer_.Step();
  if (increment) {
    wave_duty_position_ = (wave_duty_position_ + 1) % 8;
  }
}

double Apu::PulseChannel::GetDacOutput() const {
  if (!is_dac_enabled_ || !is_enabled_) {
    return 0;
  }
  unsigned wave_duty_level =
      wave_duty_table[wave_duty_pattern_][wave_duty_position_];  // {0, 1}
  unsigned volume = envelope_.GetCurrentVolume();  // {0, 1, ..., 15}
  unsigned dac_input = wave_duty_level * volume;   // {0, 1, ..., 15}
  double dac_output = (dac_input / 7.5) - 1.0;     // [-1.0, 1.0]
  return dac_output;
}

std::uint8_t Apu::WaveChannel::GetNr34() const {
  std::uint8_t result = length_timer_.IsEnabled() ? 1 << 6 : 0;
  result |= 0xBF;  // オープンのビット
  return result;
}

void Apu::WaveChannel::SetNr34(std::uint8_t value) {
  frequency_timer_.SetUpper3BitOfFrequency(value & 0x7);
  value >>= 6;
  if ((value & 1) != 0) {
    length_timer_.TurnOn();
  } else {
    length_timer_.TurnOff();
  }
  value >>= 1;
  if (value == 1) {
    Trigger();
    is_enabled_ = is_dac_enabled_;
  }
}

void Apu::WaveChannel::StepFrequencyTimer() {
  bool increment = frequency_timer_.Step();
  if (increment) {
    wave_position_ = (wave_position_ + 1) % 32;
  }
}

void Apu::WaveChannel::StepLengthTimer() {
  bool channel_off_signal = length_timer_.Step();
  if (channel_off_signal) {
    is_enabled_ = false;
  }
}

unsigned Apu::WaveChannel::GetWaveSample() const {
  unsigned sample = wave_ram_->at(wave_position_ / 2);
  if (wave_position_ % 2 == 0) {
    sample = (sample >> 4) & 0x0F;
  } else {
    sample &= 0x0F;
  }
  return sample;
}

unsigned Apu::WaveChannel::ApplyVolume(unsigned sample,
                                       Apu::WaveChannel::Volume volume) {
  switch (volume) {
    case kWaveVolumeMute:
      return 0;
    case kWaveVolume100:
      return sample;
    case kWaveVolume50:
      return sample >> 1;
    case kWaveVolume25:
      return sample >> 2;
    default:
      UNREACHABLE("Invalid enum value.")
  }
}

double Apu::WaveChannel::GetDacOutput() const {
  if (!is_dac_enabled_ || !is_enabled_) {
    return 0;
  }
  unsigned sample = GetWaveSample();
  unsigned dac_input = ApplyVolume(sample, volume_);
  double dac_output = (dac_input / 7.5) - 1.0;  // [-1.0, 1.0]
  return dac_output;
}
// NR52のビット7を0にしたときの処理
// Wave RAMはクリアされない
void Apu::ResetApu() {
  channel1_ = PulseChannel();
  channel2_ = PulseChannel();
  channel3_ = WaveChannel(wave_ram_);
  channel4_ = NoiseChannel();
  nr50_.set(0);
  nr51_.set(0);
}

void Apu::WaveChannel::Trigger() { length_timer_.Trigger(); }

std::uint8_t Apu::NoiseChannel::GetNr42() const {
  std::uint8_t result = 0;
  result |= envelope_.GetPeriod();
  if (envelope_.IsUpward()) {
    result |= 1 << 3;
  }
  result |= envelope_.GetInitialVolume() << 4;
  return result;
}

void Apu::NoiseChannel::SetNr42(std::uint8_t value) {
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

std::uint8_t Apu::NoiseChannel::GetNr43() const {
  unsigned result = clock_divider_;
  result |= lfsr_width_ << 3;
  result |= clock_shift_ << 4;
  return result;
}

void Apu::NoiseChannel::SetNr43(std::uint8_t value) {
  clock_divider_ = value & 0b111;
  value >>= 3;
  lfsr_width_ = static_cast<LfsrWidth>(value & 1);
  value >>= 1;
  clock_shift_ = value & 0xF;
}

std::uint8_t Apu::NoiseChannel::GetNr44() const {
  std::uint8_t result = length_timer_.IsEnabled() ? 1 << 6 : 0;
  result |= 0xBF;  // オープンのビット
  return result;
}

void Apu::NoiseChannel::SetNr44(std::uint8_t value) {
  value >>= 6;
  if ((value & 1) != 0) {
    length_timer_.TurnOn();
  } else {
    length_timer_.TurnOff();
  }
  value >>= 1;
  if (value == 1) {
    Trigger();
    is_enabled_ = is_dac_enabled_;
  }
}

void Apu::NoiseChannel::Trigger() {
  length_timer_.Trigger();
  envelope_.Trigger();
  lfsr_ = 0xFFFF;
}

namespace {

unsigned GetReloadedTimerValue(unsigned divider, unsigned shift) {
  return (divider > 0 ? (divider << 4) : 8) << shift;
}

}  // namespace

void Apu::NoiseChannel::StepFrequencyTimer() {
  static unsigned timer = GetReloadedTimerValue(clock_divider_, clock_shift_);

  if (--timer == 0) {
    timer = GetReloadedTimerValue(clock_divider_, clock_shift_);

    unsigned xor_result = (lfsr_ & 1) ^ ((lfsr_ & 0b10) >> 1);
    lfsr_ &= ~(1 << 15);
    lfsr_ |= xor_result << 15;
    if (lfsr_width_ == kLfsr7Bit) {
      lfsr_ &= ~(1 << 7);
      lfsr_ |= xor_result << 7;
    }
    lfsr_ >>= 1;
  }
}

void Apu::NoiseChannel::StepLengthTimer() {
  bool channel_off_signal = length_timer_.Step();
  if (channel_off_signal) {
    is_enabled_ = false;
  }
}

void Apu::NoiseChannel::StepEnvelope() { envelope_.Step(); }

double Apu::NoiseChannel::GetDacOutput() const {
  if (!is_dac_enabled_ || !is_enabled_) {
    return 0;
  }
  unsigned level = lfsr_ & 1;
  unsigned volume = envelope_.GetCurrentVolume();
  unsigned dac_input = level * volume;
  double dac_output = (dac_input / 7.5) - 1.0;
  return dac_output;
}

void Apu::Step() {
  if (!is_apu_enabled_) {
    return;
  }

  channel1_.StepFrequencyTimer();
  channel2_.StepFrequencyTimer();
  channel3_.StepFrequencyTimer();
  channel4_.StepFrequencyTimer();
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
        channel3_.StepLengthTimer();
        channel4_.StepLengthTimer();
        break;
      case 1:
        break;
      case 2:
        channel1_.StepLengthTimer();
        channel1_.StepSweep();
        channel2_.StepLengthTimer();
        channel3_.StepLengthTimer();
        channel4_.StepLengthTimer();
        break;
      case 3:
        break;
      case 4:
        channel1_.StepLengthTimer();
        channel2_.StepLengthTimer();
        channel3_.StepLengthTimer();
        channel4_.StepLengthTimer();
        break;
      case 5:
        break;
      case 6:
        channel1_.StepLengthTimer();
        channel1_.StepSweep();
        channel2_.StepLengthTimer();
        channel3_.StepLengthTimer();
        channel4_.StepLengthTimer();
        break;
      case 7:
        channel1_.StepEnvelope();
        channel2_.StepEnvelope();
        channel4_.StepEnvelope();
        break;
      default:
        UNREACHABLE("Invalid Position!");
    }
  }

  static int sample_counter = 95;
  if (--sample_counter == 0) {
    sample_counter = 95;
    PushSample();
  }
}

void Apu::Run(unsigned tcycles) {
  for (unsigned i = 0; i < tcycles; i++) {
    Step();
  }
}

void Apu::PushSample() {
  // 左の音をミックスする
  double mixed_volume_left[4] = {};
  mixed_volume_left[0] =
      nr51_.IsChannel1LeftEnabled() ? channel1_.GetDacOutput() : 0;
  mixed_volume_left[1] =
      nr51_.IsChannel2LeftEnabled() ? channel2_.GetDacOutput() : 0;
  mixed_volume_left[2] =
      nr51_.IsChannel3LeftEnabled() ? channel3_.GetDacOutput() : 0;
  mixed_volume_left[3] =
      nr51_.IsChannel4LeftEnabled() ? channel4_.GetDacOutput() : 0;
  double left_sample = (mixed_volume_left[0] + mixed_volume_left[1] +
                        mixed_volume_left[2] + mixed_volume_left[3]) /
                       4.0 * nr50_.GetLeftVolume();

  // 右の音をミックスする
  double mixed_volume_right[4] = {};
  mixed_volume_right[0] =
      nr51_.IsChannel1RightEnabled() ? channel1_.GetDacOutput() : 0;
  mixed_volume_right[1] =
      nr51_.IsChannel2RightEnabled() ? channel2_.GetDacOutput() : 0;
  mixed_volume_right[2] =
      nr51_.IsChannel3RightEnabled() ? channel3_.GetDacOutput() : 0;
  mixed_volume_right[3] =
      nr51_.IsChannel4RightEnabled() ? channel4_.GetDacOutput() : 0;
  double right_sample = (mixed_volume_right[0] + mixed_volume_right[1] +
                         mixed_volume_right[2] + mixed_volume_right[3]) /
                        4.0 * nr50_.GetRightVolume();

  audio_.PushSample(left_sample, right_sample);
}
