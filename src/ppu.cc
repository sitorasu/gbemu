#include "ppu.h"

#include <cstdint>
#include <vector>

#include "utils.h"

using namespace gbemu;

std::uint8_t Ppu::ReadVRam8(std::uint16_t address) const {
  if (ppu_mode_ == PpuMode::kDrawingPixels) {
    return 0xFF;
  }
  return vram_.at(GetVRamAddressOffset(address));
}

void Ppu::WriteVRam8(std::uint16_t address, std::uint8_t value) {
  if (ppu_mode_ != PpuMode::kDrawingPixels) {
    vram_.at(GetVRamAddressOffset(address)) = value;
  }
}

std::uint8_t Ppu::ReadOam8(std::uint16_t address) const {
  switch (ppu_mode_) {
    case PpuMode::kOamScan:
    case PpuMode::kDrawingPixels:
      return 0xFF;
    default:
      return oam_.at(GetOamAddressOffset(address));
  }
}

void Ppu::WriteOam8(std::uint16_t address, std::uint8_t value) {
  switch (ppu_mode_) {
    case PpuMode::kOamScan:
    case PpuMode::kDrawingPixels:
      break;
    default:
      oam_.at(GetOamAddressOffset(address)) = value;
      break;
  }
}

void Ppu::ScanSingleObject() {
  // 描画可能なオブジェクトの最大数に達しているなら何もしない
  if (objects_on_scan_line_.size() == kMaxNumOfObjectsOnScanLine) {
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
  if (object.IsOnScanLine(ly_, GetCurrentObjectSize())) {
    objects_on_scan_line_.push(object);
  }

  // 消費したサイクル数だけ進める
  elapsed_cycles_in_current_mode_ += 2;
}

void Ppu::WriteCurrentLineToBuffer() {
  // とりあえずオブジェクトだけ書いてみる
  while (!objects_on_scan_line_.empty()) {
    const Object& object = objects_on_scan_line_.top();
    WriteObjectOnCurrentLine(object);
    objects_on_scan_line_.pop();
  }
}

// サイクル数はM-cycle単位（＝4の倍数のT-cycle）でしか渡されず、
// Step()が返す消費サイクル数は1か2なので、貪欲に消費しても
// サイクル数が余ることはない。
void Ppu::Run(unsigned tcycle) {
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
        if ((ly_ + 1) == kLcdVerticalPixelNum) {
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
  unsigned mode_num = static_cast<unsigned>(mode);
  stat_ = (stat_ | mode_num) & mode_num;

  // 割り込みフラグを更新
  if (mode == PpuMode::kVBlank) {
    interrupt_.SetIfBit(InterruptSource::kVblank);
  } else {
    unsigned mode_interrupt_select_bit = 1U << (mode_num + 3);
    if (lcdc_ | mode_interrupt_select_bit) {
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
    stat_ |= (1U << 2);
    // 割り込みフラグを更新する
    if (stat_ | (1U << 6)) {
      interrupt_.SetIfBit(InterruptSource::kStat);
    }
  }
}

void gbemu::Ppu::WriteObjectOnCurrentLine(const Object& object) {
  // とりあえずオブジェクトだけ書いてみる
  GbLcdPixelLine& line = buffer_.at(ly_);
  int lcd_x = object.x_pos - 8;
  int lcd_y = object.y_pos - 16;
  int tile_row_offset = ly_ - lcd_y;
  int tile_data_address = object.tile_index * 16;
  std::uint8_t* tile_row_data =
      &vram_.at(tile_data_address + tile_row_offset * 2);
  std::uint8_t lower_bits = *tile_row_data;
  std::uint8_t upper_bits = *(tile_row_data + 1);
  for (int i = 0; i < 8; i++) {
    // フリップがない
    unsigned lower_bit = (lower_bits >> (7 - i)) & 1;
    unsigned upper_bit = (upper_bits >> (7 - i)) & 1;
    unsigned color_id = (upper_bit << 1) | lower_bit;
    GbObjectPalette palette = (object.attributes | (1 << 4))
                                  ? GbObjectPalette::kObp0
                                  : GbObjectPalette::kObp1;
    GbPixelColor color = GetGbObjectColor(color_id, palette);
    line.at(lcd_x + i) = color;
  }
}

GbPixelColor gbemu::Ppu::GetGbObjectColor(unsigned color_id,
                                          GbObjectPalette palette) const {
  std::uint8_t obp = palette == GbObjectPalette::kObp0 ? obp0_ : obp1_;
  return static_cast<GbPixelColor>((obp >> (color_id * 2)) & 0b11);
}

bool Ppu::Object::IsOnScanLine(std::uint8_t ly, Ppu::ObjectSize size) const {
  unsigned height = size == ObjectSize::kSingle ? 8 : 16;
  return (x_pos > 0) && (ly + 16 >= y_pos) && (ly + 16 < y_pos + height);
}
