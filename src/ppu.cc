#include "ppu.h"

#include <cstdint>
#include <vector>

#include "utils.h"

using namespace gbemu;

namespace {

// VRAM領域の開始アドレス$8000をベースとするオフセットを得る。
// $8000より小さいアドレスを引数に指定してはいけない。
static std::uint16_t GetVRamAddressOffset(std::uint16_t address) {
  return address - kVRamStartAddress;
}
// OAM領域の開始アドレス$FE00をベースとするオフセットを得る。
// $FE00より小さいアドレスを引数に指定してはいけない。
static std::uint16_t GetOamAddressOffset(std::uint16_t address) {
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
  if (objects_on_scan_line_.size() == kMaxNumOfObjectsOnScanLine) {
    elapsed_cycles_in_current_mode_ += 2;
    return;
  }

  // OAMからオブジェクトを取得
  int object_index = elapsed_cycles_in_current_mode_ / 2;
  Object object;
  object.y_pos = oam_.at(object_index * 4);
  object.x_pos = oam_.at(object_index * 4 + 1);
  object.tile_index = oam_.at(object_index * 4 + 2);
  object.attributes = oam_.at(object_index * 4 + 3);

  // オブジェクトがスキャンライン上にあるならバッファに追加
  if (object.IsOnScanLine(ly_, lcdc_.GetCurrentObjectSize())) {
    objects_on_scan_line_.push(object);
  }

  // サイクルを消費
  elapsed_cycles_in_current_mode_ += 2;
}

void Ppu::WriteCurrentLineToBuffer() {
  // とりあえずオブジェクトだけ書いてみる
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
  switch (ppu_mode_) {
    case PpuMode::kOamScan:
      ScanSingleObject();
      if (elapsed_cycles_in_current_mode_ == kOamScanDuration) {
        SetPpuMode(PpuMode::kDrawingPixels);
      }
      return 2;
    case PpuMode::kDrawingPixels:
      if (elapsed_cycles_in_current_mode_ == 0) {
        WriteCurrentLineToBuffer();
      }
      elapsed_cycles_in_current_mode_++;
      if (elapsed_cycles_in_current_mode_ == kDrawingPixelsDuration) {
        SetPpuMode(PpuMode::kHBlank);
      }
      return 1;
    case PpuMode::kHBlank:
      elapsed_cycles_in_current_mode_++;
      if (elapsed_cycles_in_current_mode_ == kHBlankDuration) {
        if ((ly_ + 1) == lcd::kHeight) {
          SetPpuMode(PpuMode::kVBlank);
          is_buffer_ready_ = true;
        } else {
          SetPpuMode(PpuMode::kOamScan);
        }
        IncrementLy();
      }
      return 1;
    case PpuMode::kVBlank:
      elapsed_cycles_in_current_mode_++;
      if (elapsed_cycles_in_current_mode_ == kVBlankDuration) {
        if ((ly_ + 1) == kScanLineNum) {
          SetPpuMode(PpuMode::kOamScan);
          is_buffer_ready_ = false;
        } else {
          // 最後の行に達するまではVBlankのまま
          elapsed_cycles_in_current_mode_ = 0;
        }
        IncrementLy();
      }
      return 1;
  }
}

void Ppu::SetPpuMode(PpuMode mode) {
  // PPUモードを設定
  ppu_mode_ = mode;

  // 経過サイクル数をリセット
  elapsed_cycles_in_current_mode_ = 0;

  // STATレジスタを更新
  stat_.SetPpuMode(mode);

  // 割り込みフラグを更新
  if (mode == PpuMode::kVBlank) {
    interrupt_.SetIfBit(InterruptSource::kVblank);
  }
  if (mode != PpuMode::kDrawingPixels) {
    if (stat_.IsPpuModeInterruptEnabled(mode)) {
      interrupt_.SetIfBit(InterruptSource::kStat);
    }
  }
}

void Ppu::IncrementLy() {
  // LYをインクリメントする
  ly_++;

  // 最後の行の場合は0に戻す
  if (ly_ == kScanLineNum) {
    ly_ = 0;
  }

  // STATレジスタを更新する
  if (lyc_ == ly_) {
    stat_.SetLycEqualsLyBit();
    // 割り込みフラグを更新する
    if (stat_.IsLycInterruptEnabled()) {
      interrupt_.SetIfBit(InterruptSource::kStat);
    }
  } else {
    stat_.ResetLycEqualsLyBit();
  }
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
  std::uint8_t* tile_row_data =
      &vram_.at(tile_data_address + tile_row_offset * 2);
  std::uint8_t lower_bits = *tile_row_data;
  std::uint8_t upper_bits = *(tile_row_data + 1);
  for (int i = 0; i < 8; i++) {
    // x-flipしているなら最下位ビットから、していないなら最上位ビットから描画する
    unsigned shift_amount = object.IsXFlip() ? i : (7 - i);
    unsigned lower_bit = (lower_bits >> shift_amount) & 1;
    unsigned upper_bit = (upper_bits >> shift_amount) & 1;
    unsigned color_id = (upper_bit << 1) | lower_bit;
    lcd::GbLcdColor color = GetGbObjectColor(color_id, palette);
    line.at(lcd_x + i) = color;
  }
}

lcd::GbLcdColor Ppu::GetGbObjectColor(unsigned color_id,
                                      Object::GbPalette palette) const {
  std::uint8_t obp = palette == Object::GbPalette::kObp0 ? obp0_ : obp1_;
  return static_cast<lcd::GbLcdColor>((obp >> (color_id * 2)) & 0b11);
}

bool Ppu::Object::IsOnScanLine(std::uint8_t ly, Ppu::ObjectSize size) const {
  unsigned height = size == ObjectSize::kSingle ? 8 : 16;
  return (x_pos > 0) && (ly + 16 >= y_pos) && (ly + 16 < y_pos + height);
}
