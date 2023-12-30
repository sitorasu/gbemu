#include "ppu.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "utils.h"

using namespace gbemu;

namespace {

// VRAM領域の開始アドレス$8000をベースとするオフセットを得る。
// $8000より小さいアドレスを引数に指定してはいけない。
std::uint16_t GetVRamAddressOffset(std::uint16_t address) {
  return address - kVRamStartAddress;
}
// OAM領域の開始アドレス$FE00をベースとするオフセットを得る。
// $FE00より小さいアドレスを引数に指定してはいけない。
std::uint16_t GetOamAddressOffset(std::uint16_t address) {
  return address - kOamStartAddress;
}

}  // namespace

std::uint8_t Ppu::ReadVRam8(std::uint16_t address) const {
  if (!IsVRamAccessible()) {
    return 0xFF;
  }
  return vram_.at(GetVRamAddressOffset(address));
}

void Ppu::WriteVRam8(std::uint16_t address, std::uint8_t value) {
  if (IsVRamAccessible()) {
    vram_.at(GetVRamAddressOffset(address)) = value;
  }
}

std::uint8_t Ppu::ReadOam8(std::uint16_t address) const {
  if (!IsOamAccessible()) {
    return 0xFF;
  }
  return oam_.at(GetOamAddressOffset(address));
}

void Ppu::WriteOam8(std::uint16_t address, std::uint8_t value) {
  if (IsOamAccessible()) {
    oam_.at(GetOamAddressOffset(address)) = value;
  }
}

void Ppu::ScanNextOamEntry() {
  // 描画可能なオブジェクトの最大数に達しているなら何もしない
  if (scanned_oam_entries_.size() == kMaxNumOfObjectsOnScanline) {
    return;
  }

  // OAMからエントリを取得
  unsigned elapsed_cycles_in_oam_scan =
      elapsed_cycles_in_frame_ % kScanlineDuration;
  ASSERT(elapsed_cycles_in_oam_scan < kOamScanDuration,
         "This function must be called within OAM Scan.");
  int entry_index = elapsed_cycles_in_oam_scan / 2;
  OamEntry entry;
  entry.y_pos = oam_.at(entry_index * 4);
  entry.x_pos = oam_.at(entry_index * 4 + 1);
  entry.tile_index = oam_.at(entry_index * 4 + 2);
  entry.attributes = oam_.at(entry_index * 4 + 3);

  // エントリが表すオブジェクトがスキャンライン上にあるならバッファに追加
  if (entry.IsOnScanline(ly_, lcdc_.GetObjectSize())) {
    scanned_oam_entries_.push_back(entry);
  }
}

void Ppu::WriteCurrentLineToBuffer() {
  std::array<unsigned, lcd::kWidth> background_color_ids{};
  if (lcdc_.IsBackgroundEnabled()) {
    WriteBackgroundOnScanline(background_color_ids);
  }

  std::array<unsigned, lcd::kWidth> window_color_ids{};
  if (lcdc_.IsWindowEnabled()) {
    // WriteWindowOnScanline(window_color_ids);
  }

  std::array<unsigned, lcd::kWidth> objects_color_ids{};
  std::array<const OamEntry*, lcd::kWidth> oam_entries{};
  if (lcdc_.IsObjectEnabled()) {
    WriteObjectsOnScanline(objects_color_ids, oam_entries);
  }

  MergeLinesOfEachLayer(background_color_ids, window_color_ids,
                        objects_color_ids, oam_entries, buffer_.at(ly_));
}

// サイクル数はM-cycle単位（＝4の倍数のT-cycle）でしか渡されず、
// Step()が返す消費サイクル数は1か2なので、貪欲に消費しても
// サイクル数が余ることはない。
void Ppu::Run(unsigned tcycle) {
  if (!lcdc_.IsPPUEnabled()) {
    return;
  }
  ASSERT(tcycle % 4 == 0, "T-cycle must be a multiple of 4");
  unsigned left_cycles = tcycle;
  while (left_cycles > 0) {
    unsigned cycles = Step();
    ASSERT(cycles <= left_cycles, "Too many cycles used");
    left_cycles -= cycles;
  }
  ASSERT(left_cycles == 0, "Cycles will never remain");
}

unsigned Ppu::Step() {
  unsigned elapsed;

  // モード固有の処理を行い、消費したサイクル数を求める
  switch (ppu_mode_) {
    case PpuMode::kOamScan:
      ScanNextOamEntry();
      elapsed = 2;
      break;
    case PpuMode::kDrawingPixels: {
      unsigned elapsed_cycles_in_line =
          elapsed_cycles_in_frame_ % kScanlineDuration;
      if (elapsed_cycles_in_line == kOamScanDuration) {
        WriteCurrentLineToBuffer();
        scanned_oam_entries_.clear();
      }
      elapsed = 1;
      break;
    }
    default:
      elapsed = 1;
      break;
  }

  // サイクルを消費する
  elapsed_cycles_in_frame_ += elapsed;
  elapsed_cycles_in_frame_ %= kFrameDuration;

  unsigned elapsed_cycles_in_line =
      elapsed_cycles_in_frame_ % kScanlineDuration;

  // LYを更新する
  if (elapsed_cycles_in_line == 0) {
    ly_ = (ly_ + 1) % kScanlineNum;
  }

  // モードを更新する
  if (ly_ < lcd::kHeight) {
    if (elapsed_cycles_in_line == 0) {
      ppu_mode_ = PpuMode::kOamScan;
    } else if (elapsed_cycles_in_line == kOamScanDuration) {
      ppu_mode_ = PpuMode::kDrawingPixels;
    } else if (elapsed_cycles_in_line ==
               (kOamScanDuration + kDrawingPixelsDuration)) {
      ppu_mode_ = PpuMode::kHBlank;
    }
  } else if (ly_ == lcd::kHeight) {
    ppu_mode_ = PpuMode::kVBlank;

    // VBlankに入ったらバッファの準備完了フラグを立てる
    is_buffer_ready_ = true;
  }

  // STATを更新する
  if (lyc_ == ly_) {
    stat_.SetLycEqualsLyBit();
  } else {
    stat_.ResetLycEqualsLyBit();
  }
  stat_.SetPpuModeBits(ppu_mode_);

  // 割り込みフラグを立てる
  bool lyc_equals_ly_interrupt = (lyc_ == ly_) && stat_.IsLycInterruptEnabled();
  bool ppu_mode_interrupt = (ppu_mode_ != PpuMode::kDrawingPixels) &&
                            stat_.IsPpuModeInterruptEnabled(ppu_mode_);
  bool stat_interrupt = lyc_equals_ly_interrupt || ppu_mode_interrupt;
  bool is_rising = !stat_interrupt_wire_ && stat_interrupt;
  if (is_rising) {
    interrupt_.SetIfBit(InterruptSource::kStat);
  }
  stat_interrupt_wire_ = stat_interrupt;
  if (ppu_mode_ == PpuMode::kVBlank) {
    interrupt_.SetIfBit(InterruptSource::kVblank);
  }

  return elapsed;
}

namespace {

// カラーIDとパレットのレジスタの値から表示すべき色を得る（ゲームボーイ用）
lcd::GbLcdColor GetGbLcdColor(unsigned color_id, std::uint8_t palette_reg) {
  return static_cast<lcd::GbLcdColor>((palette_reg >> (color_id * 2)) & 0b11);
}

}  // namespace

const std::uint8_t* Ppu::GetTileFromTileMap(int tile_pos_x, int tile_pos_y,
                                            TileMapArea area,
                                            TileDataAddressingMode mode) const {
  // タイルマップからデータを取得する
  int tile_map_index = 32 * tile_pos_y + tile_pos_x;
  std::uint16_t tile_map_base_address = area == TileMapArea::kLowerArea
                                            ? kLowerTileMapBaseAddress
                                            : kUpperTileMapBaseAddress;
  std::uint8_t tile_map_data =
      vram_.at(GetVRamAddressOffset(tile_map_base_address + tile_map_index));

  // 指定したアドレッシングモードでタイルのアドレスを計算する
  std::uint16_t tile_data_address;
  if (mode == TileDataAddressingMode::kLowerBlocksUnsigned) {
    tile_data_address = kLowerTileBlocksBaseAddress + tile_map_data * 16;
  } else {
    int offset = tile_map_data < 128 ? tile_map_data : tile_map_data - 256;
    tile_data_address = kUpperTileBlocksBaseAddress + offset * 16;
  }

  // 計算したアドレスにあるタイルへのポインタを返す
  return &vram_.at(GetVRamAddressOffset(tile_data_address));
}

std::array<unsigned, Ppu::kTileSize> Ppu::DecodeTileRow(
    const std::uint8_t* tile, unsigned row) const {
  ASSERT(row < 16, "Unexpected argument.");
  std::array<unsigned, kTileSize> result;
  const std::uint8_t* tile_row = tile + 2 * row;
  std::uint8_t lower_bits = *tile_row;
  std::uint8_t upper_bits = *(tile_row + 1);
  for (int i = 0; i < kTileSize; i++) {
    unsigned shift_amount = 7 - i;
    unsigned lower_bit = (lower_bits >> shift_amount) & 1;
    unsigned upper_bit = (upper_bits >> shift_amount) & 1;
    unsigned color_id = (upper_bit << 1) | lower_bit;
    result[i] = color_id;
  }
  return result;
}

void Ppu::MergeLinesOfEachLayer(
    const std::array<unsigned, lcd::kWidth>& background_color_ids,
    const std::array<unsigned, lcd::kWidth>& window_color_ids,
    const std::array<unsigned, lcd::kWidth>& object_color_ids,
    const std::array<const OamEntry*, lcd::kWidth>& oam_entries,
    GbLcdPixelRow& merged) {
  for (int i = 0; i < lcd::kWidth; i++) {
    lcd::GbLcdColor& dst = merged.at(i);
    dst = lcd::GbLcdColor::kWhite;

    // Backgroundの描画
    if (lcdc_.IsBackgroundEnabled()) {
      dst = GetGbLcdColor(background_color_ids[i], bgp_);
    }

    // Objectの描画
    if (lcdc_.IsObjectEnabled() && oam_entries[i] != nullptr &&
        object_color_ids[i] != 0) {
      const OamEntry& entry = *oam_entries[i];
      bool is_front = entry.GetPriority() == OamEntry::Priority::kFront;
      bool background_overlay =
          lcdc_.IsBackgroundEnabled() && background_color_ids[i] != 0;
      bool window_overlay = lcdc_.IsWindowEnabled() && window_color_ids[i] != 0;
      if (is_front || (!background_overlay && !window_overlay)) {
        OamEntry::GbPalette palette = entry.GetGbPalette();
        std::uint8_t obp =
            palette == OamEntry::GbPalette::kObp0 ? obp0_ : obp1_;
        dst = GetGbLcdColor(object_color_ids[i], obp);
      }
    }
  }
}

void Ppu::WriteBackgroundOnScanline(
    std::array<unsigned, lcd::kWidth>& color_ids) const {
  // スキャンラインと交差するタイルのうち左端のものを取得する
  int tile_pos_y = (scy_ + ly_) / kTileSize;
  int tile_pos_x = scx_ / kTileSize;
  int tile_row_offset = (scy_ + ly_) % kTileSize;
  int tile_col_offset = scx_ % kTileSize;
  TileMapArea area = lcdc_.GetBackgroundTileMapArea();
  TileDataAddressingMode mode = lcdc_.GetTileDataAddressingMode();
  const std::uint8_t* tile =
      GetTileFromTileMap(tile_pos_x, tile_pos_y, area, mode);

  // 左から右にタイルマップを移動しながら、
  // スキャンラインと交差するタイルの行を配列に書き込む
  int pos_in_line = 0;
  for (int i = 0; i < 21; i++) {
    int col_start = i == 0 ? tile_col_offset : 0;
    int col_end = i == 20 ? tile_col_offset : kTileSize;
    std::array<unsigned, kTileSize> tile_row =
        DecodeTileRow(tile, tile_row_offset);
    for (int j = col_start; j < col_end; j++) {
      color_ids.at(pos_in_line++) = tile_row[j];
    }
    tile_pos_x = (tile_pos_x + 1) % 32;
    tile = GetTileFromTileMap(tile_pos_x, tile_pos_y, area, mode);
  }
}

void Ppu::WriteObjectsOnScanline(
    std::array<unsigned, lcd::kWidth>& color_ids,
    std::array<const OamEntry*, lcd::kWidth>& oam_entries) {
  // X座標の小さいものを前面に描画するために、X座標の大きいものを先に描画する
  std::stable_sort(scanned_oam_entries_.begin(), scanned_oam_entries_.end());
  for (auto i = scanned_oam_entries_.rbegin(), e = scanned_oam_entries_.rend();
       i != e; i++) {
    WriteSingleObjectOnScanline(*i, color_ids, oam_entries);
  }
}

void Ppu::WriteSingleObjectOnScanline(
    const OamEntry& entry, std::array<unsigned, lcd::kWidth>& color_ids,
    std::array<const OamEntry*, lcd::kWidth>& oam_entries) const {
  // 描画すべきタイルの行データを取得
  int lcd_x = entry.x_pos - 8;
  int lcd_y = entry.y_pos - 16;
  int lcd_y_offset = ly_ - lcd_y;
  int tile_row_offset = entry.IsYFlip()
                            ? (lcdc_.GetObjectHeight() - 1 - lcd_y_offset)
                            : lcd_y_offset;
  int upper_tile_index = lcdc_.GetObjectSize() == ObjectSize::kDouble
                             ? entry.tile_index & 0xFE
                             : entry.tile_index;
  int tile_data_address = upper_tile_index * 16;
  const std::uint8_t* tile = &vram_.at(tile_data_address);
  std::array<unsigned, kTileSize> tile_row =
      DecodeTileRow(tile, tile_row_offset);

  // 描画
  for (int i = 0; i < 8; i++) {
    // x-flipしているなら右端から、していないなら左端から描画する
    unsigned color_id = entry.IsXFlip() ? tile_row[7 - i] : tile_row[i];

    // カラーID0は透過
    if (color_id == 0) {
      continue;
    }

    // 座標がLCDの画面内に収まるピクセルだけを描画する
    int pixel_x = lcd_x + i;
    if (0 <= pixel_x && pixel_x < lcd::kWidth) {
      color_ids.at(lcd_x + i) = color_id;
      oam_entries.at(lcd_x + i) = &entry;
    }
  }
}

bool Ppu::OamEntry::IsOnScanline(std::uint8_t ly, Ppu::ObjectSize size) const {
  unsigned height = size == ObjectSize::kSingle ? 8 : 16;
  return (ly + 16 >= y_pos) && (ly + 16 < y_pos + height);
}
