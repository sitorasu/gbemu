#ifndef GBEMU_PPU_H_
#define GBEMU_PPU_H_

#include <array>
#include <cstdint>
#include <vector>

#include "interrupt.h"
#include "utils.h"

namespace gbemu {

namespace lcd {

enum GbLcdColor {
  kWhite = 0,
  kLightGray = 1,
  kDarkGray = 2,
  kBlack = 3,
  kColorNum,
};

constexpr auto kWidth = 160;
constexpr auto kHeight = 144;
constexpr auto kTotalPixelNum = kWidth * kHeight;

}  // namespace lcd

using GbLcdPixelRow = std::array<lcd::GbLcdColor, lcd::kWidth>;
using GbLcdPixelMatrix = std::array<GbLcdPixelRow, lcd::kHeight>;

class Ppu {
 public:
  Ppu(Interrupt& interrupt)
      : vram_(kVRamSize), oam_(kOamSize), interrupt_(interrupt) {}

  void set_lcdc(std::uint8_t value) { lcdc_.data = value; }
  void set_stat(std::uint8_t value) { stat_.Set(value); }
  void set_scy(std::uint8_t value) { scy_ = value; }
  void set_scx(std::uint8_t value) { scx_ = value; }
  void set_lyc(std::uint8_t value) { lyc_ = value; }
  void set_bgp(std::uint8_t value) { bgp_ = value; }
  void set_obp0(std::uint8_t value) { obp0_ = value; }
  void set_obp1(std::uint8_t value) { obp1_ = value; }
  void set_wy(std::uint8_t value) { wy_ = value; }
  void set_wx(std::uint8_t value) { wx_ = value; }
  std::uint8_t lcdc() const { return lcdc_.data; }
  std::uint8_t stat() const { return stat_.Get(); }
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
  // バッファへの描画が完了したかどうかを調べる。
  bool IsBufferReady() const { return is_buffer_ready_; }
  // バッファへの描画完了フラグをリセットする。
  void ResetBufferReadyFlag() { is_buffer_ready_ = false; }
  // バッファを取得する。
  const GbLcdPixelMatrix& GetBuffer() const { return buffer_; }

 private:
  static constexpr auto kVRamSize = 8 * 1024;
  static constexpr auto kOamSize = 160;

  // 各モードの長さ（サイクル数）。
  // DrawingPixelsとHBlankの長さは実際には一定ではないが、
  // 実装を単純にするため一定とする。
  static constexpr auto kScanlineDuration = 456;
  static constexpr auto kOamScanDuration = 80;
  static constexpr auto kDrawingPixelsDuration = 172;
  static constexpr auto kHBlankDuration = 204;

  // Scanlineの数。
  static constexpr auto kScanlineNum = 154;

  // 1フレームの長さ（サイクル数）。
  static constexpr auto kFrameDuration = kScanlineDuration * kScanlineNum;

  // タイルの1辺のピクセル数。
  static constexpr auto kTileSize = 8;

  // タイルマップのベースアドレス。
  static constexpr auto kLowerTileMapBaseAddress = 0x9800;
  static constexpr auto kUpperTileMapBaseAddress = 0x9C00;

  // タイルデータ格納領域のベースアドレス。
  static constexpr auto kLowerTileBlocksBaseAddress = 0x8000;
  static constexpr auto kUpperTileBlocksBaseAddress = 0x9000;

  // スキャンライン上に同時に描画可能なオブジェクトの最大数
  static constexpr auto kMaxNumOfObjectsOnScanline = 10;

  // タイルマップの区画の種別。
  enum class TileMapArea {
    kLowerArea,  // $9800-9BFF
    kUpperArea   // $9C00-9FFF
  };

  // タイルデータ参照時のアドレッシングモードの種別。
  enum class TileDataAddressingMode {
    kUpperBlocksSigned,   // $9000 + signed 8-bit offset
    kLowerBlocksUnsigned  // $8000 + unsigned 8-bit offset
  };

  // オブジェクトのサイズの種別。
  enum class ObjectSize {
    kSingle,  // 8x8
    kDouble   // 8x16
  };

  // PPUモードの種別。
  enum class PpuMode {
    kHBlank = 0,
    kVBlank = 1,
    kOamScan = 2,
    kDrawingPixels = 3
  };

  // LCDCレジスタ
  struct Lcdc {
    std::uint8_t data{};

    // PPUが有効化されているか調べる。
    bool IsPPUEnabled() const { return data & (1 << 7); }

    // Windowに使用されるタイルマップの区画の種別を取得する。
    TileMapArea GetWindowTileMapArea() const {
      return data & (1 << 6) ? TileMapArea::kUpperArea
                             : TileMapArea::kLowerArea;
    }

    // Backgroundに使用されるタイルマップの区画の種別を取得する。
    TileMapArea GetBackgroundTileMapArea() const {
      return data & (1 << 3) ? TileMapArea::kUpperArea
                             : TileMapArea::kLowerArea;
    }

    // Windowレイヤーが描画されるか調べる。
    bool IsWindowEnabled() const { return (data & 1) && (data & (1 << 5)); }

    // Backgroundレイヤーが描画されるか調べる。
    bool IsBackgroundEnabled() const { return data & 1; }

    // タイルマップからタイルデータを参照する際のアドレッシングモードの種別を取得する。
    TileDataAddressingMode GetTileDataAddressingMode() const {
      return (data & (1 << 4)) ? TileDataAddressingMode::kLowerBlocksUnsigned
                               : TileDataAddressingMode::kUpperBlocksSigned;
    }

    // オブジェクトのサイズ種別を取得する。
    ObjectSize GetObjectSize() const {
      return (data & (1 << 2)) ? ObjectSize::kDouble : ObjectSize::kSingle;
    }

    // オブジェクトの高さを取得する。
    unsigned GetObjectHeight() const {
      return GetObjectSize() == ObjectSize::kDouble ? 2 * kTileSize : kTileSize;
    }

    // Objectレイヤーが描画されるかどうか調べる。
    bool IsObjectEnabled() const { return data & (1 << 1); }
  };

  // STATレジスタ
  struct Stat {
   public:
    // レジスタに値をセットする。
    // 書き込み可能なビットであるビット3~6だけをセットし、それ以外のビットは変化させない。
    void Set(std::uint8_t value) { data_ = (data_ & ~0x78) | (value & 0x78); }

    // レジスタの値を取得する。
    std::uint8_t Get() const { return data_; }

    // LYC=LYの割り込みが有効かどうか調べる。
    bool IsLycInterruptEnabled() const { return data_ & (1 << 6); }

    // PPUモードの割り込みが有効かどうか調べる。
    bool IsPpuModeInterruptEnabled(PpuMode mode) const {
      ASSERT(mode != PpuMode::kDrawingPixels,
             "STAT does not have mode 3 interrupt selection");
      std::uint8_t mode_num = static_cast<std::uint8_t>(mode);
      return data_ & (1 << (mode_num + 3));
    }

    // LYC=LYのビットを立てる。
    void SetLycEqualsLyBit() { data_ |= (1 << 2); }
    // LYC=LYのビットを下ろす。
    void ResetLycEqualsLyBit() { data_ &= ~(1 << 2); }

    // PPUモードのビットを、与えられたPPUモードを表すパターンにセットする。
    void SetPpuModeBits(PpuMode mode) {
      std::uint8_t mode_num = static_cast<std::uint8_t>(mode);
      data_ = (data_ & ~0b11) | mode_num;
    }

   private:
    std::uint8_t data_;
  };

  // OAMのエントリ
  struct OamEntry {
    std::uint8_t y_pos;
    std::uint8_t x_pos;
    std::uint8_t tile_index;
    std::uint8_t attributes;

    // オブジェクト同士の大小比較。X座標の小さい方が小さいとする。
    // オブジェクト同士の描画の優先順位を決定するために定義する。
    bool operator<(const OamEntry& rhs) const { return x_pos < rhs.x_pos; }

    // 指定のスキャンライン上で描画の対象となるか判定する。
    bool IsOnScanline(std::uint8_t ly, ObjectSize size) const;

    // Priorityの種別。
    enum class Priority {
      kFront,  // Background/Windowより前面に出る
      kBack    // Background/WindowのColor ID1-3に隠れる
    };

    // オブジェクトに紐づくパレットの種別。
    enum class GbPalette { kObp0, kObp1 };

    // attributesの各種情報の取得。
    Priority GetPriority() const {
      return (attributes & (1 << 7)) ? Priority::kBack : Priority::kFront;
    }
    bool IsYFlip() const { return attributes & (1 << 6); }
    bool IsXFlip() const { return attributes & (1 << 5); }
    GbPalette GetGbPalette() const {
      return (attributes & (1 << 4)) ? GbPalette::kObp1 : GbPalette::kObp0;
    }
  };

  // 要素数NのカラーIDの配列。
  // 要素はカラーIDを表す整数。
  // ただし-1は透過を表す。
  template <int N>
  using ColorIdArray = std::array<int, N>;

  // 次のOAMエントリをスキャンする。
  void ScanNextOamEntry();

  // 現在の行をバッファに書き込む。
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

  // VRAMがアクセス可能かどうか調べる
  bool IsVRamAccessible() const {
    return !lcdc_.IsPPUEnabled() || (ppu_mode_ != PpuMode::kDrawingPixels);
  }

  // OAMがアクセス可能かどうか調べる
  bool IsOamAccessible() const {
    return !lcdc_.IsPPUEnabled() || (ppu_mode_ == PpuMode::kHBlank) ||
           (ppu_mode_ == PpuMode::kVBlank);
  }

  // タイルマップの指定の位置のタイルへのポインタを返す
  const std::uint8_t* GetTileFromTileMap(int tile_pos_x, int tile_pos_y,
                                         TileMapArea area,
                                         TileDataAddressingMode mode) const;

  // タイルの任意の行をデコードしてカラーIDの配列にする。
  // 行は0〜15で指定できる。8以上を指定できるのは高さ16のオブジェクトのタイルに対応するため。
  // 戻り値の配列の0番目がタイルの左端のカラーIDに対応する。
  ColorIdArray<kTileSize> DecodeTileRow(const std::uint8_t* tile,
                                        unsigned row) const;

  // スキャンラインに沿ってBackgroundレイヤの各ピクセルのカラーIDを配列に書き出す。
  void WriteBackgroundOnScanline(ColorIdArray<lcd::kWidth>& color_ids) const;

  // スキャンラインに沿ってWindowレイヤの各ピクセルのカラーIDを配列に書き出す。
  void WriteWindowOnScanline(ColorIdArray<lcd::kWidth>& color_ids) const;

  // スキャンラインに沿ってObjectレイヤの各ピクセルのカラーIDとそのピクセルが属する
  // オブジェクトのOAMエントリを配列に書き出す。
  // オブジェクトのない部分に対応する配列要素は変化させない。
  void WriteObjectsOnScanline(
      ColorIdArray<lcd::kWidth>& color_ids,
      std::array<const OamEntry*, lcd::kWidth>& oam_entries);

  // WriteObjectsOnCurrentLineのヘルパ。
  // 1つのOAMエントリに対応するピクセルの情報を配列に書き出す。
  void WriteSingleObjectOnScanline(
      const OamEntry& entry, ColorIdArray<lcd::kWidth>& color_ids,
      std::array<const OamEntry*, lcd::kWidth>& oam_entries) const;

  // スキャンラインに沿って抽出した各レイヤをマージしてLCDの1行として書き出す
  void MergeLinesOfEachLayer(
      const ColorIdArray<lcd::kWidth>& background_color_ids,
      const ColorIdArray<lcd::kWidth>& window_color_ids,
      const ColorIdArray<lcd::kWidth>& object_color_ids,
      const std::array<const OamEntry*, lcd::kWidth>& oam_entries,
      GbLcdPixelRow& merged);

  Lcdc lcdc_{};
  Stat stat_{};
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

  PpuMode ppu_mode_{PpuMode::kOamScan};

  // 現在のフレームにおける経過サイクル数。
  unsigned elapsed_cycles_in_frame_{};

  // バッファへの描画が完了したかどうかを示すフラグ。
  // VBlankに入るとtrueになり、VBlankが終わるか、
  // このフラグの値を外部から取得するとfalseになる。
  bool is_buffer_ready_{false};

  Interrupt& interrupt_;

  // STAT割り込みの信号線の状態を記憶する。
  // STAT割り込みの要求フラグはこの信号線がlow(false)から
  // high(true)になったときセットされる。
  bool stat_interrupt_wire_{false};

  // OAM Scanで使用するバッファ。
  // 現在のスキャンライン上で描画の対象となるオブジェクトをX座標の小さい順に格納する。
  std::vector<OamEntry> scanned_oam_entries_{};
};

}  // namespace gbemu

#endif  // GBEMU_PPU_H_
