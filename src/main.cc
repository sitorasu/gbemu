#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

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

CartridgeHeader::CartridgeHeader(std::string title, CartridgeTarget target,
                                 CartridgeType type, std::uint32_t rom_size,
                                 std::uint32_t ram_size)
    : title(title),
      target(target),
      type(type),
      rom_size(rom_size),
      ram_size(ram_size) {}

CartridgeHeader CartridgeHeader::create(std::uint8_t* rom) {
  // $0134-0143（title, 16バイト）を取得
  std::string title(reinterpret_cast<const char*>(&rom[0x134]), 16);

  // $0143（CGB flag）を取得
  // このアドレスはtitleの末尾と被っている。意味のある値が入っている場合はtitleの末尾を削除する。
  CartridgeTarget target = [&title, rom]() {
    std::uint8_t value = rom[0x143];
    switch (value) {
      case 0x80:
        title.pop_back();
        return CartridgeTarget::kGbAndGbc;
      case 0xC0:
        title.pop_back();
        return CartridgeTarget::kGbc;
      default:
        if (isprint(value) || (value == 0x00)) {
          return CartridgeTarget::kGb;
        } else {
          std::cerr << "Cartridge target error at $0143: "
                    << static_cast<int>(value) << "\n";
          std::exit(0);
        }
    }
  }();

  // $0147（Cartridge type）を取得
  CartridgeType type = [rom]() {
    std::uint8_t value = rom[0x147];
    switch (value) {
      case 0x00:
        return CartridgeType::kRomOnly;
      case 0x01:
        return CartridgeType::kMbc1;
      case 0x03:
        return CartridgeType::kMbc1RamBattery;
      default:
        std::cerr << "Cartridge type error at $0147: "
                  << static_cast<int>(value) << "\n";
        std::exit(0);
    }
  }();

  // $0148（ROM size）を取得
  std::uint32_t rom_size = [rom]() {
    std::uint8_t value = rom[0x148];
    if (0 <= value && value <= 8) {
      return (32 << value);
    } else {
      std::cerr << "Cartridge ROM size error at $0148: "
                << static_cast<int>(value) << "\n";
      std::exit(0);
    }
  }();

  // $0149（RAM size）を取得
  std::uint32_t ram_size = [rom]() {
    std::uint8_t value = rom[0x149];
    switch (value) {
      case 0x00:
        return 0;
      case 0x02:
        return 8;
      case 0x03:
        return 32;
      case 0x04:
        return 128;
      case 0x05:
        return 64;
      default:
        std::cerr << "Cartridge RAM size error at $0149: "
                  << static_cast<int>(value) << "\n";
        std::exit(0);
    }
  }();

  return CartridgeHeader(title, target, type, rom_size, ram_size);
}

std::string CartridgeHeader::getCartridgeTargetString(CartridgeTarget target) {
  switch (target) {
    case CartridgeTarget::kGb:
      return "GB";
    case CartridgeTarget::kGbAndGbc:
      return "GB/GBC";
    case CartridgeTarget::kGbc:
      return "GBC";
  }
}

std::string CartridgeHeader::getCartridgeTypeString(CartridgeType type) {
  switch (type) {
    case CartridgeType::kRomOnly:
      return "ROM Only";
    case CartridgeType::kMbc1:
      return "MBC1";
    case CartridgeType::kMbc1RamBattery:
      return "MBC1+RAM+BATTERY";
    default:
      return "Unknown";
  }
}

void CartridgeHeader::print() {
  std::cout << "title: " << title << "\n";
  std::cout << "target: " << getCartridgeTargetString(target) << "\n";
  std::cout << "type: " << getCartridgeTypeString(type) << "\n";
  std::cout << "rom_size: " << rom_size << " KiB\n";
  std::cout << "ram_size: " << ram_size << " KiB\n";
}

int main(int argc, char* argv[]) {
  // コマンドラインの書式をチェック
  if (argc < 2) {
    std::cerr << "usage: gbemu ROM_FILE\n";
    return 0;
  }

  // ROMファイルをオープン
  std::string path = argv[1];
  std::ifstream ifs(path, std::ios_base::in | std::ios_base::binary);
  if (ifs.fail()) {
    std::cerr << "File open error: " << path << "\n";
    return 0;
  }

  // ROMファイルを読み出す
  std::istreambuf_iterator<char> it_ifs_begin(ifs);
  std::istreambuf_iterator<char> it_ifs_end{};
  std::vector<char> input_data(it_ifs_begin, it_ifs_end);
  if (ifs.fail()) {
    std::cerr << "File read error: " << path << "\n";
    return 0;
  }

  // ROMファイルをクローズ
  ifs.close();
  if (ifs.fail()) {
    std::cerr << "File close error: " << path << "\n";
    return 0;
  }

  if (input_data.size() < 150) {
    std::cerr << "ROM format error\n";
    return 0;
  }

  CartridgeHeader header =
      CartridgeHeader::create(reinterpret_cast<uint8_t*>(input_data.data()));
  header.print();

  switch (header.type) {
    case CartridgeHeader::CartridgeType::kRomOnly:
      break;
    case CartridgeHeader::CartridgeType::kMbc1:
      break;
    default:
      std::cerr << "Not implemented cartridge type.\n";
      return 0;
  }

  return 0;
}
