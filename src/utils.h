#ifndef GBEMU_UTILS_H_
#define GBEMU_UTILS_H_

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace gbemu {

// プログラムの設計が正しければ成り立つはずの条件を表明するマクロ。
// 表明した条件が成り立たなければエラーを報告してプログラムを終了する。
// 異常系のエラーとして使用すること。
// 第1引数に条件、第2引数にエラーメッセージのフォーマット文字列、
// 第3引数以降にフォーマット文字列に埋め込む値を指定する。
// エラーの文字列の末尾は改行される。
// 使用例: ASSERT(x > 0, "Invalid value: %d", x);
#define ASSERT(cond, ...)                                       \
  if (!(cond)) {                                                \
    std::fprintf(stderr, "System error (Assertion failed):\n"); \
    std::fprintf(stderr, "  FILE: %s\n", __FILE__);             \
    std::fprintf(stderr, "  LINE: %d\n", __LINE__);             \
    std::fprintf(stderr, "  ");                                 \
    std::fprintf(stderr, __VA_ARGS__);                          \
    std::fprintf(stderr, "\n");                                 \
    std::exit(1);                                               \
  }

// プログラムの設計が正しければ到達しない制御フローを表明するマクロ。
// エラーを報告してプログラムを終了する。
// 異常系のエラーとして使用すること。
// 第1引数にエラーメッセージのフォーマット文字列、
// 第2引数以降にフォーマット文字列に埋め込む値を指定する。
// エラーの文字列の末尾は改行される。
// 使用例: UNREACHABLE("Invalid state: %d", x);
#define UNREACHABLE(...)                                   \
  {                                                        \
    std::fprintf(stderr, "System error (Unreachable):\n"); \
    std::fprintf(stderr, "  FILE: %s\n", __FILE__);        \
    std::fprintf(stderr, "  LINE: %d\n", __LINE__);        \
    std::fprintf(stderr, "  ");                            \
    std::fprintf(stderr, __VA_ARGS__);                     \
    std::fprintf(stderr, "\n");                            \
    std::exit(1);                                          \
  }

// 警告を出すマクロ。
// 使用例: WARN("Waning: %d", x);
#define WARN(...)                                   \
  {                                                 \
    std::fprintf(stderr, "System warning:\n");      \
    std::fprintf(stderr, "  FILE: %s\n", __FILE__); \
    std::fprintf(stderr, "  LINE: %d\n", __LINE__); \
    std::fprintf(stderr, "  ");                     \
    std::fprintf(stderr, __VA_ARGS__);              \
    std::fprintf(stderr, "\n");                     \
  }

// エラーを報告してプログラムを終了する。
// 正常系のエラーとして使用すること。
// 第1引数にエラーメッセージのフォーマット文字列、
// 第2引数以降にフォーマット文字列に埋め込む値を指定する。
// エラーの文字列の末尾は改行される。
[[noreturn]] void Error(const char* fmt, ...);

std::uint16_t ConcatUInt(std::uint8_t lower, std::uint8_t upper);

// 第posビットからbitsビット分を切り出す
std::uint8_t ExtractBits(std::uint8_t value, unsigned pos, unsigned bits);

// xが区間[begin, end)に含まれるならtrue、そうでなければfalseを返す
template <class T1, class T2, class T3>
inline bool InRange(T1 x, T2 begin, T3 end) {
  return begin <= x && x < end;
}

// 各メモリ領域のアドレスの定義。
// k*StartAddressは領域に含まれ、k*EndAddressは領域に含まれない。
static constexpr std::uint16_t kRomStartAddress = 0x0000;
static constexpr std::uint16_t kRomEndAddress = 0x8000;
static constexpr std::uint16_t kVRamStartAddress = 0x8000;
static constexpr std::uint16_t kVRamEndAddress = 0xA000;
static constexpr std::uint16_t kExternalRamStartAddress = 0xA000;
static constexpr std::uint16_t kExternalRamEndAddress = 0xC000;
static constexpr std::uint16_t kInternalRamStartAddress = 0xC000;
static constexpr std::uint16_t kInternalRamEndAddress = 0xE000;
static constexpr std::uint16_t kEchoRamStartAddress = 0xE000;
static constexpr std::uint16_t kEchoRamEndAddress = 0xFE00;
static constexpr std::uint16_t kOamStartAddress = 0xFE00;
static constexpr std::uint16_t kOamEndAddress = 0xFEA0;
static constexpr std::uint16_t kNotUsableAreaStartAddress = 0xFEA0;
static constexpr std::uint16_t kNotUsableAreaEndAddress = 0xFF00;
static constexpr std::uint16_t kIORegistersStartAddress = 0xFF00;
static constexpr std::uint16_t kIORegistersEndAddress = 0xFF80;
static constexpr std::uint16_t kHRamStartAddress = 0xFF80;
static constexpr std::uint16_t kHRamEndAddress = 0xFFFF;

inline bool InRomRange(std::uint16_t address) {
  return InRange(address, kRomStartAddress, kRomEndAddress);
}

inline bool InVRamRange(std::uint16_t address) {
  return InRange(address, kVRamStartAddress, kVRamEndAddress);
}

inline bool InExternalRamRange(std::uint16_t address) {
  return InRange(address, kExternalRamStartAddress, kExternalRamEndAddress);
}

inline bool InInternalRamRange(std::uint16_t address) {
  return InRange(address, kInternalRamStartAddress, kInternalRamEndAddress);
}

inline bool InEchoRamRange(std::uint16_t address) {
  return InRange(address, kEchoRamStartAddress, kEchoRamEndAddress);
}

inline bool InOamRange(std::uint16_t address) {
  return InRange(address, kOamStartAddress, kOamEndAddress);
}

inline bool InNotUsableAreaRange(std::uint16_t address) {
  return InRange(address, kNotUsableAreaStartAddress, kNotUsableAreaEndAddress);
}

inline bool InIORegistersRange(std::uint16_t address) {
  return InRange(address, kIORegistersStartAddress, kIORegistersEndAddress);
}

inline bool InHRamRange(std::uint16_t address) {
  return InRange(address, kHRamStartAddress, kHRamEndAddress);
}

}  // namespace gbemu

#endif  // GBEMU_UTILS_H_
