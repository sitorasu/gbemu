#ifndef GBEMU_PPU_H_
#define GBEMU_PPU_H_

#include <array>
#include <cstdint>
#include <queue>
#include <vector>

#include "interrupt.h"

namespace gbemu {

namespace gb {
namespace lcd {
enum Color {
  kWhite = 0,
  kLightGray = 1,
  kDarkGray = 2,
  kBlack = 3,
  kColorNum,
};
}
}  // namespace gb

constexpr auto kLcdHorizontalPixelNum = 160;
constexpr auto kLcdVerticalPixelNum = 144;
constexpr auto kLcdTotalPixelNum =
    kLcdHorizontalPixelNum * kLcdVerticalPixelNum;

using GbLcdPixelLine = std::array<gb::lcd::Color, kLcdHorizontalPixelNum>;
using GbLcdPixelMatrix = std::array<GbLcdPixelLine, kLcdVerticalPixelNum>;

class Ppu {
 public:
  Ppu(Interrupt& interrupt)
      : vram_(kVRamSize), oam_(kOamSize), interrupt_(interrupt) {}

  void set_lcdc(std::uint8_t value) { lcdc_ = value; }
  void set_stat(std::uint8_t value) { stat_ = value & 0x78; }
  void set_scy(std::uint8_t value) { scy_ = value; }
  void set_scx(std::uint8_t value) { scx_ = value; }
  void set_lyc(std::uint8_t value) { lyc_ = value; }
  void set_bgp(std::uint8_t value) { bgp_ = value; }
  void set_obp0(std::uint8_t value) { obp0_ = value; }
  void set_obp1(std::uint8_t value) { obp1_ = value; }
  void set_wy(std::uint8_t value) { wy_ = value; }
  void set_wx(std::uint8_t value) { wx_ = value; }
  std::uint8_t lcdc() const { return lcdc_; }
  std::uint8_t stat() const { return stat_; }
  std::uint8_t scy() const { return scy_; }
  std::uint8_t scx() const { return scx_; }
  std::uint8_t ly() const { return ly_; }
  std::uint8_t lyc() const { return lyc_; }
  std::uint8_t bgp() const { return bgp_; }
  std::uint8_t obp0() const { return obp0_; }
  std::uint8_t obp1() const { return obp1_; }
  std::uint8_t wy() const { return wy_; }
  std::uint8_t wx() const { return wx_; }

  // VRAMから値を読み出す。
  // 引数はCPUから見たメモリ上のアドレス。
  // PPUのモードがDrawing Pixelsのときは0xFFを返す。
  std::uint8_t ReadVRam8(std::uint16_t address) const;
  // VRAMに値を書き込む。
  // 引数はCPUから見たメモリ上のアドレスと、書き込む値。
  // PPUのモードがDrawing Pixelsのときは何もしない。
  void WriteVRam8(std::uint16_t address, std::uint8_t value);
  // OAMから値を読み出す。
  // 引数はCPUから見たメモリ上のアドレス。
  // PPUのモードがOAM ScanまたはDrawing Pixelsのときは0xFFを返す。
  std::uint8_t ReadOam8(std::uint16_t address) const;
  // OAMに値を書き込む。
  // 引数はCPUから見たメモリ上のアドレスと、書き込む値。
  // PPUのモードがOAM ScanまたはDrawing Pixelsのときは何もしない。
  void WriteOam8(std::uint16_t address, std::uint8_t value);
  // 指定されたサイクル数だけPPUを進める。
  void Run(unsigned tcycle);
  // バッファへの描画が完了した
  bool IsBufferReady() {
    if (is_buffer_ready_) {
      is_buffer_ready_ = false;
      return true;
    }
    return false;
  }
  // バッファを取得する
  const GbLcdPixelMatrix& GetBuffer() const { return buffer_; }

 private:
  static constexpr auto kVRamSize = 8 * 1024;
  static constexpr auto kOamSize = 160;

  // 各モードの長さ（サイクル数）。
  // DrawingPixelsとHBlankの長さは実際には一定ではないが、
  // 実装を単純にするため一定とする。
  // VBlankは1行区切りとする。
  static constexpr auto kOamScanDuration = 80;
  static constexpr auto kDrawingPixelsDuration = 172;
  static constexpr auto kHBlankDuration = 204;
  static constexpr auto kVBlankDuration = 456;

  // VBlankも含めたScan Lineの総数
  static constexpr auto kScanLineNum = 154;

  // VRAM領域の開始アドレス$8000をベースとするオフセットを得る。
  // $8000より小さいアドレスを引数に指定してはいけない。
  static std::uint16_t GetVRamAddressOffset(std::uint16_t address) {
    return address - 0x8000U;
  }
  // OAM領域の開始アドレス$FE00をベースとするオフセットを得る。
  // $FE00より小さいアドレスを引数に指定してはいけない。
  static std::uint16_t GetOamAddressOffset(std::uint16_t address) {
    return address - 0xFE00U;
  }

  // PPUが有効化されているか調べる。
  bool IsPPUEnabled() const { return lcdc_ & (1 << 7); }

  // Background/Windowに使用するタイルマップの区画
  enum class TileMapArea {
    kLowerArea,  // $9800-9BFF
    kUpperArea   // $9C00-9FFF
  };
  // 現在のWindowのタイルマップの区画を取得する
  TileMapArea GetCurrentWindowTileMapArea() const {
    return lcdc_ & (1 << 6) ? TileMapArea::kUpperArea : TileMapArea::kLowerArea;
  }
  // 現在のBackgroundのタイルマップの区画を取得する
  TileMapArea GetCurrentBackgroundTileMapArea() const {
    return lcdc_ & (1 << 3) ? TileMapArea::kUpperArea : TileMapArea::kLowerArea;
  }

  // Windowレイヤーが描画されるかどうか調べる
  bool IsWindowEnabled() const { return (lcdc_ & 1) && (lcdc_ & (1 << 5)); }

  // Backgroundレイヤーが描画されるかどうか調べる
  bool IsBackgroundEnabled() const { return lcdc_ & 1; }

  // Background/Windowに使用するタイルデータへのアドレッシングモード
  enum class TileDataAddressingMode {
    kUpperBlocksSigned,   // $9000 + signed 8-bit offset
    kLowerBlocksUnsigned  // $8000 + unsigned 8-bit offset
  };
  // Background/Windowに使用するタイルデータへのアドレッシングモードを取得する
  TileDataAddressingMode GetCurrentTileDataAddressingMode() const {
    return (lcdc_ & (1 << 4)) ? TileDataAddressingMode::kLowerBlocksUnsigned
                              : TileDataAddressingMode::kUpperBlocksSigned;
  }

  enum class ObjectSize {
    kSingle,  // 8x8
    kDouble   // 8x16
  };
  // 現在のオブジェクトのサイズ種別を取得する
  ObjectSize GetCurrentObjectSize() const {
    return (lcdc_ & (1 << 2)) ? ObjectSize::kDouble : ObjectSize::kSingle;
  }
  // 現在のオブジェクトの高さを取得する
  unsigned GetCurrentObjectHeight() const {
    return GetCurrentObjectSize() == ObjectSize::kDouble ? 16 : 8;
  }

  // Objectレイヤーが描画されるかどうか調べる
  bool IsObjectEnabled() const { return lcdc_ & (1 << 1); }

  // OAMのオブジェクトを1つスキャンし、2サイクル進める。
  void ScanSingleObject();

  // 現在の行をバッファに書き込む
  void WriteCurrentLineToBuffer();

  // PPUを最小の処理単位で進め、消費したサイクル数を返す。
  // 最小の処理単位はPPUのモードごとに以下のように定める。
  // - OAM Scan: 1つのオブジェクトをスキャンする（2サイクル）
  // - Drawing Pixels: このモードに入ると同時に現在の行全体を描画し、
  //                   後は何もしない（1サイクル）(*)
  // - HBlank: 何もしない（1サイクル）
  // - VBlank: 何もしない（1サイクル）
  // (*) Drawing Pixelsの実際の動作はそうではないが、
  //     実装が大変なので単純化する。
  unsigned Step();

  enum class PpuMode { kHBlank, kVBlank, kOamScan, kDrawingPixels };
  PpuMode ppu_mode_{PpuMode::kOamScan};

  // PPUモードを設定する。
  // 経過サイクル数のリセットと、STATレジスタおよび割り込みフラグの更新も行う。
  void SetPpuMode(PpuMode mode);

  // VRAMがアクセス可能かどうか調べる
  bool IsVRamAccessible() const {
    return !IsPPUEnabled() || (ppu_mode_ != PpuMode::kDrawingPixels);
  }

  // OAMがアクセス可能かどうか調べる
  bool IsOamAccessible() const {
    return !IsPPUEnabled() || (ppu_mode_ == PpuMode::kHBlank) ||
           (ppu_mode_ == PpuMode::kVBlank);
  }

  // LYレジスタをインクリメントする。
  // 最後の行の場合は0に戻す。
  // STATレジスタと割り込みフラグの更新も行う。
  void IncrementLy();

  // 現在のモードにおける経過サイクル数。
  // VBlankでは1行ごとにリセットされる。
  unsigned elapsed_cycles_in_current_mode_{};

  std::uint8_t lcdc_{};
  std::uint8_t stat_{};
  std::uint8_t scy_{};
  std::uint8_t scx_{};
  std::uint8_t ly_{};
  std::uint8_t lyc_{};
  std::uint8_t bgp_{};
  std::uint8_t obp0_{};
  std::uint8_t obp1_{};
  std::uint8_t wy_{};
  std::uint8_t wx_{};
  std::vector<std::uint8_t> vram_;
  std::vector<std::uint8_t> oam_;
  GbLcdPixelMatrix buffer_{};  // LCDに表示する画面
  // バッファへの描画が完了したかどうかを示すフラグ。
  // VBlankに入るとtrueになり、VBlankが終わるか、
  // このフラグの値を外部から取得されるとfalseになる。
  bool is_buffer_ready_{false};

  Interrupt& interrupt_;

  struct Object {
    std::uint8_t y_pos;
    std::uint8_t x_pos;
    std::uint8_t tile_index;
    std::uint8_t attributes;
    bool operator>(const Object& rhs) const { return x_pos > rhs.x_pos; }
    // 指定のスキャンライン上で描画の対象となるか判定する
    bool IsOnScanLine(std::uint8_t ly, ObjectSize size) const;

    enum class Priority {
      kFront,  // Background/Windowより前面に出る
      kBack    // Background/WindowのColor ID1-3に隠れる
    };
    enum class GbPalette { kObp0, kObp1 };
    // attributesの各種情報の取得
    Priority GetPriority() const {
      return (attributes & (1 << 7)) ? Priority::kBack : Priority::kFront;
    }
    bool IsYFlip() const { return attributes & (1 << 6); }
    bool IsXFlip() const { return attributes & (1 << 5); }
    GbPalette GetGbPalette() const {
      return (attributes & (1 << 4)) ? GbPalette::kObp1 : GbPalette::kObp0;
    }
  };
  // OAM Scanで使用するバッファ。
  // 現在のスキャンライン上で描画の対象となるオブジェクトを
  // X座標の小さい順に格納する。
  std::priority_queue<Object, std::vector<Object>, std::greater<Object>>
      objects_on_scan_line_{};
  // スキャンライン上に同時に描画可能なオブジェクトの最大数
  static constexpr auto kMaxNumOfObjectsOnScanLine = 10;

  // OAM Scanでバッファに記録した全オブジェクトをスキャンライン上に描画する。
  void WriteObjectsOnCurrentLine();

  // 指定のオブジェクトをスキャンライン上に描画する。
  void WriteSingleObjectOnCurrentLine(const Object& object);

  gb::lcd::Color GetGbObjectColor(unsigned color_id,
                                  Object::GbPalette palette) const;
};

}  // namespace gbemu

#endif  // GBEMU_PPU_H_
