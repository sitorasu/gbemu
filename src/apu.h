#ifndef GBEMU_APU_H_
#define GBEMU_APU_H_

#include <array>
#include <cstdint>

namespace gbemu {

class Apu {
 public:
  std::uint8_t get_nr10() const { return nr10_; }
  std::uint8_t get_nr11() const { return nr11_; }
  std::uint8_t get_nr12() const { return nr12_; }
  std::uint8_t get_nr13() const { return nr13_; }
  std::uint8_t get_nr14() const { return nr14_; }
  std::uint8_t get_nr21() const { return nr21_; }
  std::uint8_t get_nr22() const { return nr22_; }
  std::uint8_t get_nr23() const { return nr23_; }
  std::uint8_t get_nr24() const { return nr24_; }
  std::uint8_t get_nr30() const { return nr30_; }
  std::uint8_t get_nr31() const { return nr31_; }
  std::uint8_t get_nr32() const { return nr32_; }
  std::uint8_t get_nr33() const { return nr33_; }
  std::uint8_t get_nr34() const { return nr34_; }
  std::uint8_t get_nr41() const { return nr41_; }
  std::uint8_t get_nr42() const { return nr42_; }
  std::uint8_t get_nr43() const { return nr43_; }
  std::uint8_t get_nr44() const { return nr44_; }
  std::uint8_t get_nr50() const { return nr50_.get(); }
  std::uint8_t get_nr51() const { return nr51_.get(); }
  std::uint8_t get_nr52() const { return nr52_.get(); }
  std::uint8_t get_wave_ram(int index) const { return wave_ram_.at(index); }
  void set_nr10(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr10_ = value;
    }
  }
  void set_nr11(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr11_ = value;
    }
  }
  void set_nr12(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr12_ = value;
    }
  }
  void set_nr13(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr13_ = value;
    }
  }
  void set_nr14(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr14_ = value;
    }
  }
  void set_nr21(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr21_ = value;
    }
  }
  void set_nr22(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr22_ = value;
    }
  }
  void set_nr23(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr23_ = value;
    }
  }
  void set_nr24(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr24_ = value;
    }
  }
  void set_nr30(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr30_ = value;
    }
  }
  void set_nr31(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr31_ = value;
    }
  }
  void set_nr32(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr32_ = value;
    }
  }
  void set_nr33(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr33_ = value;
    }
  }
  void set_nr34(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr34_ = value;
    }
  }
  void set_nr41(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr41_ = value;
    }
  }
  void set_nr42(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr42_ = value;
    }
  }
  void set_nr43(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr43_ = value;
    }
  }
  void set_nr44(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr44_ = value;
    }
  }
  void set_nr50(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr50_.set(value);
    }
  }
  void set_nr51(std::uint8_t value) {
    if (nr52_.IsApuEnabled()) {
      nr51_.set(value);
    }
  }
  void set_nr52(std::uint8_t value);
  void set_wave_ram(int index, std::uint8_t value) {
    wave_ram_.at(index) = value;
  }

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

  class Nr52 {
   public:
    std::uint8_t get() const { return data_; }
    void set(std::uint8_t value) {
      data_ &= (1 << 7);
      data_ |= value & (1 << 7);
    }

    bool IsApuEnabled() const { return data_ & (1 << 7); }
    void SetChannel1OnBit() { data_ |= (1 << 0); }
    void SetChannel2OnBit() { data_ |= (1 << 1); }
    void SetChannel3OnBit() { data_ |= (1 << 2); }
    void SetChannel4OnBit() { data_ |= (1 << 3); }
    void ResetChannel1OnBit() { data_ &= ~(1 << 0); }
    void ResetChannel2OnBit() { data_ &= ~(1 << 1); }
    void ResetChannel3OnBit() { data_ &= ~(1 << 2); }
    void ResetChannel4OnBit() { data_ &= ~(1 << 3); }

   private:
    std::uint8_t data_{};
  };

  void ResetApu();

  std::uint8_t nr10_{};
  std::uint8_t nr11_{};
  std::uint8_t nr12_{};
  std::uint8_t nr13_{};
  std::uint8_t nr14_{};
  std::uint8_t nr21_{};
  std::uint8_t nr22_{};
  std::uint8_t nr23_{};
  std::uint8_t nr24_{};
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
  Nr52 nr52_{};
  std::array<std::uint8_t, 16> wave_ram_;
};

}  // namespace gbemu

#endif  // GBEMU_APU_H_
