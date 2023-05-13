#ifndef GBEMU_CARTRIDGE_HEADER_H_
#define GBEMU_CARTRIDGE_HEADER_H_

#include <cstdint>
#include <string>
#include <vector>

namespace gbemu {

// いったん生成したらメンバ変数の値は変更できない
struct CartridgeHeader {
  using uint8_t = std::uint8_t;
  using uint32_t = std::uint32_t;

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

  const uint32_t rom_size;  // 単位：KiB
  const uint32_t ram_size;  // 単位：KiB

  static CartridgeHeader Create(const std::vector<uint8_t>& rom_data);
  ~CartridgeHeader() {}

  void Print();

 private:
  explicit CartridgeHeader(const std::vector<uint8_t>& rom_data)
      : title(GetTitle(rom_data)),
        target(GetCartridgeTarget(rom_data)),
        type(GetCartridgeType(rom_data)),
        rom_size(GetRomSize(rom_data)),
        ram_size(GetRamSize(rom_data)) {}
  static std::string GetTitle(const std::vector<uint8_t>& rom_data);
  static CartridgeTarget GetCartridgeTarget(
      const std::vector<uint8_t>& rom_data);
  static CartridgeType GetCartridgeType(const std::vector<uint8_t>& rom_data);
  static uint32_t GetRomSize(const std::vector<uint8_t>& rom_data);
  static uint32_t GetRamSize(const std::vector<uint8_t>& rom_data);
  static std::string GetCartridgeTargetString(CartridgeTarget target);
  static std::string GetCartridgeTypeString(CartridgeType type);
};

}  // namespace gbemu

#endif  // GBEMU_CARTRIDGE_HEADER_H_
