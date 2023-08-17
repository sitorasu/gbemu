#ifndef GBEMU_UTILS_H_
#define GBEMU_UTILS_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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

// xが区間[begin, end)に含まれるならtrue、そうでなければfalseを返す
template <class T1, class T2, class T3>
inline bool InRange(T1 x, T2 begin, T3 end) {
  return begin <= x && x < end;
}

// ビット幅が等しい符号なし整数から符号付き整数への変換。
template <class UInt, class SInt>
inline SInt CastToSigned(UInt v) {
  constexpr SInt sint_max = std::numeric_limits<SInt>::max();
  constexpr SInt sint_min = std::numeric_limits<SInt>::min();

  static_assert(sizeof(UInt) == sizeof(SInt), "Invalid cast to signed.");
  static_assert(
      static_cast<UInt>(sint_min) == (static_cast<UInt>(sint_max) + 1),
      "Invalid SInt limits.");

  if (v <= sint_max) {
    return static_cast<SInt>(v);
  }
  return sint_min + static_cast<SInt>(v - sint_min);
}

}  // namespace gbemu

#endif  // GBEMU_UTILS_H_
