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
  std::uint8_t get_nr50() const { return nr50_; }
  std::uint8_t get_nr51() const { return nr51_; }
  std::uint8_t get_nr52() const { return nr52_; }
  std::uint8_t get_wave_ram(int index) const { return wave_ram_.at(index); }
  void set_nr10(std::uint8_t value) { nr10_ = value; }
  void set_nr11(std::uint8_t value) { nr11_ = value; }
  void set_nr12(std::uint8_t value) { nr12_ = value; }
  void set_nr13(std::uint8_t value) { nr13_ = value; }
  void set_nr14(std::uint8_t value) { nr14_ = value; }
  void set_nr21(std::uint8_t value) { nr21_ = value; }
  void set_nr22(std::uint8_t value) { nr22_ = value; }
  void set_nr23(std::uint8_t value) { nr23_ = value; }
  void set_nr24(std::uint8_t value) { nr24_ = value; }
  void set_nr30(std::uint8_t value) { nr30_ = value; }
  void set_nr31(std::uint8_t value) { nr31_ = value; }
  void set_nr32(std::uint8_t value) { nr32_ = value; }
  void set_nr33(std::uint8_t value) { nr33_ = value; }
  void set_nr34(std::uint8_t value) { nr34_ = value; }
  void set_nr41(std::uint8_t value) { nr41_ = value; }
  void set_nr42(std::uint8_t value) { nr42_ = value; }
  void set_nr43(std::uint8_t value) { nr43_ = value; }
  void set_nr44(std::uint8_t value) { nr44_ = value; }
  void set_nr50(std::uint8_t value) { nr50_ = value; }
  void set_nr51(std::uint8_t value) { nr51_ = value; }
  void set_nr52(std::uint8_t value) { nr52_ = value; }
  void set_wave_ram(int index, std::uint8_t value) {
    wave_ram_.at(index) = value;
  }

 private:
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
  std::uint8_t nr50_{};
  std::uint8_t nr51_{};
  std::uint8_t nr52_{};
  std::array<std::uint8_t, 16> wave_ram_;
};

}  // namespace gbemu

#endif  // GBEMU_APU_H_
