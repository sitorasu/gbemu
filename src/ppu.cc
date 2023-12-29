#include "ppu.h"

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

void Ppu::ScanSingleObject() {
  // 描画可能なオブジェクトの最大数に達しているなら何もせずサイクルを消費
  if (objects_on_scan_line_.size() == kMaxNumOfObjectsOnScanline) {
    return;
  }

  // OAMからオブジェクトを取得
  unsigned elapsed_cycles_in_oam_scan =
      elapsed_cycles_in_frame_ % kScanlineDuration;
  ASSERT(elapsed_cycles_in_oam_scan < kOamScanDuration,
         "This function must be called within OAM Scan.");
  int object_index = elapsed_cycles_in_oam_scan / 2;
  Object object;
  object.y_pos = oam_.at(object_index * 4);
  object.x_pos = oam_.at(object_index * 4 + 1);
  object.tile_index = oam_.at(object_index * 4 + 2);
  object.attributes = oam_.at(object_index * 4 + 3);

  // オブジェクトがスキャンライン上にあるならバッファに追加
  if (object.IsOnScanline(ly_, lcdc_.GetCurrentObjectSize())) {
    objects_on_scan_line_.push(object);
  }
}

void Ppu::WriteCurrentLineToBuffer() {
  WriteBackgroundOnCurrentLine();
  WriteWindowOnCurrentLine();
  WriteObjectsOnCurrentLine();
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
      ScanSingleObject();
      elapsed = 2;
      break;
    case PpuMode::kDrawingPixels: {
      unsigned elapsed_cycles_in_line =
          elapsed_cycles_in_frame_ % kScanlineDuration;
      if (elapsed_cycles_in_line == kOamScanDuration) {
        WriteCurrentLineToBuffer();
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
  stat_.SetPpuMode(ppu_mode_);

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

void Ppu::WriteBackgroundOnCurrentLine() {
  // 書き込み先のバッファの行
  GbLcdPixelRow& line = buffer_.at(ly_);

  // スキャンラインと交差するタイルのうち左端のものを取得する
  int tile_pos_y = (scy_ + ly_) / kTileSize;
  int tile_pos_x = scx_ / kTileSize;
  int tile_row_offset = (scy_ + ly_) % kTileSize;
  int tile_col_offset = scx_ % kTileSize;
  TileMapArea area = lcdc_.GetCurrentBackgroundTileMapArea();
  TileDataAddressingMode mode = lcdc_.GetCurrentTileDataAddressingMode();
  const std::uint8_t* tile =
      GetTileFromTileMap(tile_pos_x, tile_pos_y, area, mode);

  // 左から右にタイルマップを移動しながら、スキャンラインと交差するタイルの行を
  // バッファに書き込む
  int pos_in_line = 0;
  for (int i = 0; i < 21; i++) {
    int col_start = i == 0 ? tile_col_offset : 0;
    int col_end = i == 20 ? tile_col_offset : kTileSize;
    std::array<unsigned, kTileSize> tile_row =
        DecodeTileRow(tile, tile_row_offset);
    for (int j = col_start; j < col_end; j++) {
      lcd::GbLcdColor color = GetGbLcdColor(tile_row[j], bgp_);
      line.at(pos_in_line++) = color;
    }
    tile_pos_x = (tile_pos_x + 1) % 32;
    tile = GetTileFromTileMap(tile_pos_x, tile_pos_y, area, mode);
  }
}

void Ppu::WriteWindowOnCurrentLine() {
  // do something
}

void Ppu::WriteObjectsOnCurrentLine() {
  while (!objects_on_scan_line_.empty()) {
    if (lcdc_.IsObjectEnabled()) {
      const Object& object = objects_on_scan_line_.top();
      WriteSingleObjectOnCurrentLine(object);
    }
    // オブジェクトの表示が有効でも無効でも捨てることに注意（バグを作り込んだ）
    objects_on_scan_line_.pop();
  }
}

void Ppu::WriteSingleObjectOnCurrentLine(const Object& object) {
  GbLcdPixelRow& line = buffer_.at(ly_);
  Object::GbPalette palette = object.GetGbPalette();
  std::uint8_t obp = palette == Object::GbPalette::kObp0 ? obp0_ : obp1_;
  // TODO: Priorityの実装

  // 描画すべき行データを取得
  int lcd_x = object.x_pos - 8;
  int lcd_y = object.y_pos - 16;
  int lcd_y_offset = ly_ - lcd_y;
  int tile_row_offset =
      object.IsYFlip() ? (lcdc_.GetCurrentObjectHeight() - 1 - lcd_y_offset)
                       : lcd_y_offset;
  int upper_tile_index = lcdc_.GetCurrentObjectSize() == ObjectSize::kDouble
                             ? object.tile_index & 0xFE
                             : object.tile_index;
  int tile_data_address = upper_tile_index * 16;
  std::uint8_t* tile = &vram_.at(tile_data_address);
  std::array<unsigned, kTileSize> tile_row =
      DecodeTileRow(tile, tile_row_offset);
  for (int i = 0; i < 8; i++) {
    // x-flipしているなら右端から、していないなら左端から描画する
    unsigned color_id = object.IsXFlip() ? tile_row[7 - i] : tile_row[i];
    if (color_id == 0) {
      continue;
    }
    lcd::GbLcdColor color = GetGbLcdColor(color_id, obp);
    // TODO: object.x_pos < 8 のケースの対処
    line.at(lcd_x + i) = color;
  }
}

bool Ppu::Object::IsOnScanline(std::uint8_t ly, Ppu::ObjectSize size) const {
  unsigned height = size == ObjectSize::kSingle ? 8 : 16;
  return (x_pos > 0) && (ly + 16 >= y_pos) && (ly + 16 < y_pos + height);
}
