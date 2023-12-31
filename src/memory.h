#ifndef GBEMU_MEMORY_H_
#define GBEMU_MEMORY_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "cartridge.h"
#include "interrupt.h"
#include "ppu.h"
#include "timer.h"
#include "utils.h"

namespace gbemu {

// CPUから見たメモリを表すクラス。
// メモリアクセスを各コンポーネントに振り分ける。
class Memory {
 private:
  class Dma {
   public:
    Dma(Memory& memory) : memory_(memory) {}

    std::uint8_t dma() const { return dma_; }

    // DMAレジスタに値をセットしDMA転送を開始する。
    void StartDma(std::uint8_t value) {
      dma_ = value;
      during_transfer_ = true;
      src_address_ = (dma_ << 8);
      dst_address_ = kOamStartAddress;
    }

    // 指定したマシンサイクル数だけDMA転送を進める
    void Run(unsigned mcycles);

   private:
    Memory& memory_;
    std::uint8_t dma_{};
    bool during_transfer_{};
    std::uint16_t src_address_{};
    std::uint16_t dst_address_{kOamStartAddress};
  };

 public:
  // コンストラクタ。ブートROMを与えるとメモリにマップした状態で初期化する。
  Memory(Cartridge* cartridge, Interrupt& interrupt, Timer& timer, Ppu& ppu,
         std::vector<std::uint8_t>* boot_rom = nullptr)
      : cartridge_(cartridge),
        interrupt_(interrupt),
        timer_(timer),
        ppu_(ppu),
        dma_(*this),
        internal_ram_(kInternalRamSize),
        h_ram_(kHRamSize),
        boot_rom_(boot_rom),
        is_boot_rom_mapped_(boot_rom_ != nullptr) {}

  bool IsBootRomMapped() const { return is_boot_rom_mapped_; }

  std::uint8_t Read8(std::uint16_t address) const;
  std::uint16_t Read16(std::uint16_t address) const;
  std::vector<std::uint8_t> ReadBytes(std::uint16_t address,
                                      unsigned bytes) const;
  void Write8(std::uint16_t address, std::uint8_t value);
  void Write16(std::uint16_t address, std::uint16_t value);

  // DMAを指定のマシンサイクルだけ進める
  void RunDma(unsigned mcycles) { dma_.Run(mcycles); }

 private:
  std::uint8_t ReadIORegister(std::uint16_t address) const;
  void WriteIORegister(std::uint16_t address, std::uint8_t value);

  constexpr static auto kInternalRamSize = 8 * 1024;
  constexpr static auto kHRamSize = 127;
  Cartridge* cartridge_;
  Interrupt& interrupt_;
  Timer& timer_;
  Ppu& ppu_;
  Dma dma_;

  // ゲームボーイカラーだとRAMのサイズが違うのでarrayにはしないでおく
  std::vector<std::uint8_t> internal_ram_;
  std::vector<std::uint8_t> h_ram_;
  std::vector<std::uint8_t>* boot_rom_;

  bool is_boot_rom_mapped_;
};

}  // namespace gbemu

#endif  // GBEMU_MEMORY_H_
