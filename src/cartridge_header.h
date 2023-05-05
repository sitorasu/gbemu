#ifndef GBEMU_CARTRIDGE_HEADER_H_
#define GBEMU_CARTRIDGE_HEADER_H_

#include <cstdint>
#include <string>

namespace gbemu {

// createメソッドもしくはコピー・ムーブコンストラクタでしか生成できない
// いったん生成したらメンバ変数の値は変更できない
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

  ~CartridgeHeader() {}

  static CartridgeHeader create(std::uint8_t* rom);
  void print();

 private:
  CartridgeHeader(std::string title, CartridgeTarget target, CartridgeType type,
                  std::uint32_t rom_size, std::uint32_t ram_size);
  std::string getCartridgeTargetString(CartridgeTarget target);
  std::string getCartridgeTypeString(CartridgeType type);
};

}  // namespace gbemu

#endif  // GBEMU_CARTRIDGE_HEADER_H_
