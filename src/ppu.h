#ifndef GBEMU_PPU_H_
#define GBEMU_PPU_H_

#include <cstdint>
#include <vector>

namespace gbemu {

class Ppu {
 public:
  static constexpr auto kVRamSize = 8 * 1024;
  static constexpr auto kOamSize = 160;

  Ppu() : vram_(kVRamSize), oam_(kOamSize) {}

  void set_lcdc(std::uint8_t value) { lcdc_ = value; }
  void set_stat(std::uint8_t value) { stat_ = value & 0x78; }
  void set_scy(std::uint8_t value) { scy_ = value; }
  void set_scx(std::uint8_t value) { scx_ = value; }
  void set_lyc(std::uint8_t value) { lyc_ = value; }
  void set_dma(std::uint8_t value) { dma_ = value; }
  void set_bgp(std::uint8_t value) { bgp_ = value; }
  void set_obp0(std::uint8_t value) { obp0_ = value; }
  void set_obp1(std::uint8_t value) { obp1_ = value; }
  void set_wy(std::uint8_t value) { wy_ = value; }
  void set_wx(std::uint8_t value) { wx_ = value; }
  std::uint8_t lcdc() { return lcdc_; }
  std::uint8_t stat() { return stat_; }
  std::uint8_t scy() { return scy_; }
  std::uint8_t scx() { return scx_; }
  std::uint8_t ly() { return ly_; }
  std::uint8_t lyc() { return lyc_; }
  std::uint8_t dma() { return dma_; }
  std::uint8_t bgp() { return bgp_; }
  std::uint8_t obp0() { return obp0_; }
  std::uint8_t obp1() { return obp1_; }
  std::uint8_t wy() { return wy_; }
  std::uint8_t wx() { return wx_; }

  // VRAMから値を読み出す。
  // 引数はCPUから見たメモリ上のアドレス。
  std::uint8_t ReadVRam8(std::uint16_t address);
  // VRAMに値を書き込む。
  // 引数はCPUから見たメモリ上のアドレスと、書き込む値。
  void WriteVRam8(std::uint16_t address, std::uint8_t value);
  // OAMから値を読み出す。
  // 引数はCPUから見たメモリ上のアドレス。
  std::uint8_t ReadOam8(std::uint16_t address);
  // OAMに値を書き込む。
  // 引数はCPUから見たメモリ上のアドレスと、書き込む値。
  void WriteOam8(std::uint16_t address, std::uint8_t value);

 private:
  // VRAM領域の開始アドレス$8000をベースとするオフセットを得る。
  // $8000より小さいアドレスを引数に指定してはいけない。
  static std::uint16_t GetVRamAddressOffset(std::uint16_t address) {
    return address - 0x8000;
  }

  // OAM領域の開始アドレス$FE00をベースとするオフセットを得る。
  // $FE00より小さいアドレスを引数に指定してはいけない。
  static std::uint16_t GetOamAddressOffset(std::uint16_t address) {
    return address - 0xFE00;
  }

  std::uint8_t lcdc_{};
  std::uint8_t stat_{};
  std::uint8_t scy_{};
  std::uint8_t scx_{};
  std::uint8_t ly_{};
  std::uint8_t lyc_{};
  std::uint8_t dma_{};
  std::uint8_t bgp_{};
  std::uint8_t obp0_{};
  std::uint8_t obp1_{};
  std::uint8_t wy_{};
  std::uint8_t wx_{};
  std::vector<std::uint8_t> vram_;
  std::vector<std::uint8_t> oam_;
};

}  // namespace gbemu

#endif  // GBEMU_PPU_H_
