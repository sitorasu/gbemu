#ifndef GBEMU_CARTRIDGE_HEADER_H_
#define GBEMU_CARTRIDGE_HEADER_H_

#include <cstdint>
#include <string>
#include <vector>

namespace gbemu {

// カートリッジが動作するゲームボーイのモデル
enum class CartridgeTarget { kGb, kGbAndGbc, kGbc };

// カートリッジの構成種別
enum class CartridgeType {
  kRomOnly,
  kMbc1,
  kMbc1Ram,
  kMbc1RamBattery,
  kMbc2,
  kMbc2Battery,
  kRomRam,
  kRomRamBattery,
  kMmm01,
  kMmm01Ram,
  kMmm01RamBattery,
  kMbc3TimerBattery,
  kMbc3TimerRamBattery,
  kMbc3,
  kMbc3Ram,
  kMbc3RamBattery,
  kMbc5,
  kMbc5Ram,
  kMbc5RamBattery,
  kMbc5Rumble,
  kMbc5RumbleRam,
  kMbc5RumbleRamBattery,
  kMbc6,
  kMbc7SensorRumbleRamBattery,
  kPocketCamera,
  kBandaiTama5,
  kHuc3,
  kHuc1RamBattery
};

// カートリッジのヘッダの情報を取得・記憶する構造体。
// Example:
//   std::vector<std::uint8_t> rom = LoadRom();
//   CartridgeHeader header();
//   header.Parse(rom);
class CartridgeHeader {
 public:
  CartridgeHeader() {}

  // ROMのヘッダーをパースしメンバ変数を設定する。
  // 入力データが0x150バイトに満たない場合はエラーとしプログラムを終了する。
  void Parse(const std::vector<std::uint8_t>& rom);
  // RAMを内蔵しているか否かを返す。
  bool HasRam();

  CartridgeType type() { return type_; }
  unsigned rom_size() { return rom_size_; }
  unsigned ram_size() { return ram_size_; }

 private:
  std::string title_;
  CartridgeTarget target_;
  CartridgeType type_;
  unsigned rom_size_;  // 単位：KiB
  unsigned ram_size_;  // 単位：KiB

  // 各メンバ変数の値を標準出力に出力する（デバッグ用）
  void Print();
};

}  // namespace gbemu

#endif  // GBEMU_CARTRIDGE_HEADER_H_
