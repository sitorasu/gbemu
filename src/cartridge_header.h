#ifndef GBEMU_CARTRIDGE_HEADER_H_
#define GBEMU_CARTRIDGE_HEADER_H_

#include <cstdint>
#include <string>
#include <vector>

namespace gbemu {

// カートリッジのヘッダの情報を取得・記憶する構造体。
// Example:
//   std::vector<std::uint8_t> rom = LoadRom();
//   CartridgeHeader header = CartridgeHeader::Create(rom);
//   auto rom_size = header.rom_size;
struct CartridgeHeader {
  const std::string title;

  enum class CartridgeTarget { kGb, kGbAndGbc, kGbc };
  const CartridgeTarget target;

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
  const CartridgeType type;

  const std::uint32_t rom_size;  // 単位：KiB
  const std::uint32_t ram_size;  // 単位：KiB

  // `rom`の先頭0x150バイトをカートリッジヘッダとして読み込みインスタンスを生成する。
  // `rom`が0x150バイトに満たない場合はプログラムを終了する。
  static CartridgeHeader Create(const std::vector<std::uint8_t>& rom);
  ~CartridgeHeader() {}

  // 各メンバ変数の値を標準出力に出力する（デバッグ用）
  void Print();

 private:
  // `rom`の先頭0x150バイトをカートリッジヘッダとして読み込みインスタンスを生成する。
  // このコンストラクタを直接呼ぶのではなくCreateを使用すること。
  explicit CartridgeHeader(const std::vector<std::uint8_t>& rom)
      : title(GetTitle(rom)),
        target(GetCartridgeTarget(rom)),
        type(GetCartridgeType(rom)),
        rom_size(GetRomSize(rom)),
        ram_size(GetRamSize(rom)) {}
  // ROMのタイトルを取得する
  static std::string GetTitle(const std::vector<std::uint8_t>& rom);
  // ROMが動作するモデル（GB or GB/GBC or GBC）を取得する。
  // 取得できない場合はプログラムを終了する。
  static CartridgeTarget GetCartridgeTarget(
      const std::vector<std::uint8_t>& rom);
  // ROMのMBCの種類を取得する。
  // 取得できない場合はプログラムを終了する。
  static CartridgeType GetCartridgeType(const std::vector<std::uint8_t>& rom);
  // ROMのサイズを取得する。
  // 取得できない場合はプログラムを終了する。
  static std::uint32_t GetRomSize(const std::vector<std::uint8_t>& rom);
  // カートリッジに内蔵されたRAMのサイズを取得する。
  // 取得できない場合はプログラムを終了する。
  static std::uint32_t GetRamSize(const std::vector<std::uint8_t>& rom);
  // CartridgeTargetの値を文字列に変換する。
  static std::string GetCartridgeTargetString(CartridgeTarget target);
  // CartridgeTypeの値を文字列に変換する。
  static std::string GetCartridgeTypeString(CartridgeType type);
};

}  // namespace gbemu

#endif  // GBEMU_CARTRIDGE_HEADER_H_
