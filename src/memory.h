#ifndef GBEMU_MEMORY_H_
#define GBEMU_MEMORY_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "apu.h"
#include "cartridge.h"
#include "interrupt.h"
#include "joypad.h"
#include "ppu.h"
#include "serial.h"
#include "timer.h"
#include "utils.h"

namespace gbemu {

// CPUから見たメモリを表すクラス。
// メモリアクセスを各コンポーネントに振り分ける。
class Memory {
 private:
  // DMA転送を行うクラス。
  // DMA転送を要求されて最初のRunでは供給されるクロックが
  // DMAレジスタの代入によって生じたものであると見越して何もしない。
  // (DMAレジスタの代入が完了してからDMA転送を開始する方が正確な
  // エミュレーションであるという立場をとる。)
  class Dma {
   public:
    Dma(Memory& memory, Ppu& ppu) : memory_(memory), ppu_(ppu) {}

    std::uint8_t dma() const { return dma_; }

    // DMAレジスタに値をセットしDMA転送を要求する。
    void RequestDma(std::uint8_t value) {
      dma_ = value;
      state_ = State::kRequested;
      src_address_ = (dma_ << 8);
      dst_address_ = kOamStartAddress;
    }

    // 指定したマシンサイクル数だけDMA転送を進める。
    // ただしRequestDmaを呼び出してから最初のRunでは何もしない。
    void Run(unsigned mcycles);

   private:
    // DMA転送の状態。
    enum class State {
      kWaiting,    // DMA転送の要求待ちの状態
      kRequested,  // DMA転送が要求され、まだ開始していない状態
      kRunning     // DMA転送の最中
    };

    State state_{State::kWaiting};
    Memory& memory_;
    Ppu& ppu_;
    std::uint8_t dma_{};
    std::uint16_t src_address_{};
    std::uint16_t dst_address_{kOamStartAddress};
  };

 public:
  // コンストラクタ。ブートROMを与えるとメモリにマップした状態で初期化する。
  Memory(Cartridge* cartridge, Interrupt& interrupt, Timer& timer,
         Joypad& joypad, Serial& serial, Ppu& ppu, Apu& apu,
         std::vector<std::uint8_t>* boot_rom = nullptr)
      : cartridge_(cartridge),
        interrupt_(interrupt),
        timer_(timer),
        joypad_(joypad),
        serial_(serial),
        ppu_(ppu),
        apu_(apu),
        dma_(*this, ppu),
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
  Joypad& joypad_;
  Serial& serial_;
  Ppu& ppu_;
  Apu& apu_;
  Dma dma_;

  // ゲームボーイカラーだとRAMのサイズが違うのでarrayにはしないでおく
  std::vector<std::uint8_t> internal_ram_;
  std::vector<std::uint8_t> h_ram_;
  std::vector<std::uint8_t>* boot_rom_;

  bool is_boot_rom_mapped_;
};

}  // namespace gbemu

#endif  // GBEMU_MEMORY_H_
