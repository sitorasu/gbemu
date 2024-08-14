#ifndef GBEMU_APU_H_
#define GBEMU_APU_H_

#include <array>
#include <cstdint>
#include <vector>

namespace gbemu {

class Apu {
 public:
  std::uint8_t get_nr10() const { return channel1_.GetNrX0(); }
  std::uint8_t get_nr11() const { return channel1_.GetNrX1(); }
  std::uint8_t get_nr12() const { return channel1_.GetNrX2(); }
  std::uint8_t get_nr14() const { return channel1_.GetNrX4(); }
  std::uint8_t get_nr21() const { return channel2_.GetNrX1(); }
  std::uint8_t get_nr22() const { return channel2_.GetNrX2(); }
  std::uint8_t get_nr24() const { return channel2_.GetNrX4(); }
  std::uint8_t get_nr30() const { return nr30_ | 0x7F; }
  std::uint8_t get_nr32() const { return nr32_ | 0x9F; }
  std::uint8_t get_nr34() const { return nr34_ | 0xBF; }
  std::uint8_t get_nr42() const { return nr42_; }
  std::uint8_t get_nr43() const { return nr43_; }
  std::uint8_t get_nr44() const { return nr44_ | 0xBF; }
  std::uint8_t get_nr50() const { return nr50_.get(); }
  std::uint8_t get_nr51() const { return nr51_.get(); }
  std::uint8_t get_nr52() const;
  std::uint8_t get_wave_ram(int index) const { return wave_ram_.at(index); }

  void set_nr10(std::uint8_t value) {
    if (is_apu_enabled_) {
      channel1_.SetNrX0(value);
    }
  }
  void set_nr11(std::uint8_t value) {
    if (is_apu_enabled_) {
      channel1_.SetNrX1(value);
    }
  }
  void set_nr12(std::uint8_t value) {
    if (is_apu_enabled_) {
      channel1_.SetNrX2(value);
    }
  }
  void set_nr13(std::uint8_t value) {
    if (is_apu_enabled_) {
      channel1_.SetNrX3(value);
    }
  }
  void set_nr14(std::uint8_t value) {
    if (is_apu_enabled_) {
      channel1_.SetNrX4(value);
    }
  }
  void set_nr21(std::uint8_t value) {
    if (is_apu_enabled_) {
      channel2_.SetNrX1(value);
    }
  }
  void set_nr22(std::uint8_t value) {
    if (is_apu_enabled_) {
      channel2_.SetNrX2(value);
    }
  }
  void set_nr23(std::uint8_t value) {
    if (is_apu_enabled_) {
      channel2_.SetNrX3(value);
    }
  }
  void set_nr24(std::uint8_t value) {
    if (is_apu_enabled_) {
      channel2_.SetNrX4(value);
    }
  }
  void set_nr30(std::uint8_t value) {
    if (is_apu_enabled_) {
      nr30_ = value;
    }
  }
  void set_nr31(std::uint8_t value) {
    if (is_apu_enabled_) {
      nr31_ = value;
    }
  }
  void set_nr32(std::uint8_t value) {
    if (is_apu_enabled_) {
      nr32_ = value;
    }
  }
  void set_nr33(std::uint8_t value) {
    if (is_apu_enabled_) {
      nr33_ = value;
    }
  }
  void set_nr34(std::uint8_t value) {
    if (is_apu_enabled_) {
      nr34_ = value;
    }
  }
  void set_nr41(std::uint8_t value) {
    if (is_apu_enabled_) {
      nr41_ = value;
    }
  }
  void set_nr42(std::uint8_t value) {
    if (is_apu_enabled_) {
      nr42_ = value;
    }
  }
  void set_nr43(std::uint8_t value) {
    if (is_apu_enabled_) {
      nr43_ = value;
    }
  }
  void set_nr44(std::uint8_t value) {
    if (is_apu_enabled_) {
      nr44_ = value;
    }
  }
  void set_nr50(std::uint8_t value) {
    if (is_apu_enabled_) {
      nr50_.set(value);
    }
  }
  void set_nr51(std::uint8_t value) {
    if (is_apu_enabled_) {
      nr51_.set(value);
    }
  }
  void set_nr52(std::uint8_t value);
  void set_wave_ram(int index, std::uint8_t value) {
    if (is_apu_enabled_) {
      wave_ram_.at(index) = value;
    }
  }

  void Run(unsigned tcycles);

 private:
  class Nr50 {
   public:
    std::uint8_t get() const { return data_; }
    void set(std::uint8_t value) { data_ = value; }

    // 0b000~0b111を1/8~8/8にマップして返す
    double GetLeftVolume() const { return (((data_ >> 4) & 0b111) + 1) / 8.0; }
    double GetRightVolume() const { return ((data_ & 0b111) + 1) / 8.0; }

   private:
    std::uint8_t data_{};
  };

  class Nr51 {
   public:
    std::uint8_t get() const { return data_; }
    void set(std::uint8_t value) { data_ = value; }

    bool IsChannel1RightEnabled() const { return data_ & (1 << 0); }
    bool IsChannel2RightEnabled() const { return data_ & (1 << 1); }
    bool IsChannel3RightEnabled() const { return data_ & (1 << 2); }
    bool IsChannel4RightEnabled() const { return data_ & (1 << 3); }
    bool IsChannel1LeftEnabled() const { return data_ & (1 << 4); }
    bool IsChannel2LeftEnabled() const { return data_ & (1 << 5); }
    bool IsChannel3LeftEnabled() const { return data_ & (1 << 6); }
    bool IsChannel4LeftEnabled() const { return data_ & (1 << 7); }

   private:
    std::uint8_t data_{};
  };

  class FrameSequencer {
   public:
    unsigned GetPos() const { return pos_; }

    // 1 T-cycle分進める。
    // 8192 T-cycle目でtrueを返す。
    bool Step() {
      timer_--;
      if (timer_ == 0) {
        timer_ = 8192;
        pos_ = (pos_ + 1) & 7;
        return true;
      }
      return false;
    }

   private:
    // 8192 T-cycleをカウントするタイマー
    unsigned timer_{8192};
    // 8192 T-cycleごとに進む位置。0~7の値を繰り返す。
    unsigned pos_{};
  };

  class FrequencyTimer {
   public:
    unsigned GetUpper3BitOfFrequency() const { return (frequency_ >> 8) & 7; }
    unsigned GetLower8BitOfFrequency() const { return frequency_ & 0xFF; }
    void SetUpper3BitOfFrequency(unsigned value) {
      frequency_ &= 0xFF;
      frequency_ |= (value & 7) << 8;
    }
    void SetLower8BitOfFrequency(unsigned value) {
      frequency_ &= 0xF00;
      frequency_ |= value & 0xFF;
    }
    void SetFrequency(unsigned value) { frequency_ = value & 0x7FF; }
    unsigned GetFrequency() const { return frequency_; }

    // 1 T-cycle分進める。
    // タイマーが0になりリロードされたらtrueを返す
    bool Step() {
      frequency_timer_--;
      if (frequency_timer_ == 0) {
        frequency_timer_ = (2048 - frequency_) * 4;
        return true;
      }
      return false;
    }

   private:
    unsigned frequency_{};
    unsigned frequency_timer_{8192};
  };

  class Envelope {
   public:
    unsigned GetPeriod() const { return period_; }
    bool IsUpward() const { return is_upward_; }
    unsigned GetInitialVolume() const { return initial_volume_; }
    void SetPeriod(unsigned value) { period_ = value; }
    void SetDirection(bool is_upward) { is_upward_ = is_upward; }
    void SetInitialVolume(unsigned value) { initial_volume_ = value; }
    // Frame Sequencerから1クロック与えられたときの処理を行う
    void Step();
    // トリガー時の処理
    void Trigger();

   private:
    // Envelope開始時の音量
    unsigned initial_volume_{};
    // 音量をインクリメントするか、デクリメントするか
    bool is_upward_{};
    // Envelopeを一段階進めるのに要するFrame Sequencerのクロックの数
    unsigned period_{};
    // 現在の音量
    unsigned current_volume_{};
    // Frame Sequencerからクロックでデクリメントするタイマー
    unsigned period_timer_{};
  };

  class LengthTimer {
   public:
    void TurnOn() { is_enabled_ = true; }
    void TurnOff() { is_enabled_ = false; }
    bool IsEnabled() const { return is_enabled_; }
    void Set(unsigned value) { timer_ = value; }
    unsigned Get() const { return timer_; }
    // Frame Sequencerからの1クロック分動作する
    // タイマーのカウントが終了したらtrueを返す
    bool Step() {
      if (is_enabled_) {
        timer_--;
        if (timer_ == 0) {
          return true;
        }
      }
      return false;
    }

   private:
    bool is_enabled_{};
    unsigned timer_{};
  };

  class Sweep {
   public:
    unsigned GetFrequency() const { return current_frequency_; }
    unsigned GetPeriod() const { return period_; }
    unsigned IsUpward() const { return is_upward_; }
    unsigned GetShiftAmount() const { return shift_amount_; }
    void SetPeriod(unsigned value);
    void SetDirection(bool is_upward) { is_upward_ = is_upward; }
    void SetShiftAmount(unsigned value) { shift_amount_ = value & 0x7; }
    void SetFrequency(unsigned value) { current_frequency_ = value; }
    void Trigger(unsigned initial_frequency);

    // Frame Sequencerからの1クロック分動作する
    // 周波数がオーバーフローしたかどうかを返す
    bool Step();

   private:
    unsigned CalculateNewFrequency() const;
    unsigned period_{};
    bool is_upward_{};
    unsigned shift_amount_{};
    unsigned current_frequency_{};
    unsigned timer_{};
  };

  class PulseChannel {
   public:
    void StepSweep();
    void StepLengthTimer();
    void StepEnvelope();
    void StepFrequencyTimer();
    bool IsEnabled() const { return is_enabled_; }
    std::uint8_t GetNrX0() const;
    std::uint8_t GetNrX1() const;
    std::uint8_t GetNrX2() const;
    std::uint8_t GetNrX4() const;
    void SetNrX0(std::uint8_t value);
    void SetNrX1(std::uint8_t value);
    void SetNrX2(std::uint8_t value);
    void SetNrX3(std::uint8_t value);
    void SetNrX4(std::uint8_t value);
    void Trigger();

   private:
    Sweep sweep_{};
    LengthTimer length_timer_{};
    Envelope envelope_{};
    FrequencyTimer freqency_timer_{};
    bool is_enabled_{false};
    bool is_dac_enabled_{false};
    unsigned wave_duty_pattern_{};
    unsigned wave_duty_position_{};
  };

  static const unsigned wave_duty_table[4][8];

  void ResetApu();
  void Step();

  std::uint8_t nr30_{};
  std::uint8_t nr31_{};
  std::uint8_t nr32_{};
  std::uint8_t nr33_{};
  std::uint8_t nr34_{};
  std::uint8_t nr41_{};
  std::uint8_t nr42_{};
  std::uint8_t nr43_{};
  std::uint8_t nr44_{};
  Nr50 nr50_{};
  Nr51 nr51_{};
  std::array<std::uint8_t, 16> wave_ram_;

  bool is_apu_enabled_;

  FrameSequencer frame_sequencer_;
  PulseChannel channel1_;
  PulseChannel channel2_;

  std::vector<double> samples_{2048};
};

}  // namespace gbemu

#endif  // GBEMU_APU_H_
