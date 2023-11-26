#ifndef GBEMU_RENDERER_H_
#define GBEMU_RENDERER_H_

#include <SDL.h>

#include <array>

namespace gbemu {

// この定義はPPUに移してもいいかも
enum GBColor {
  kWhite,
  kLightGray,
  kDarkGray,
  kBlack,
  kGBColorNum,
};

constexpr auto kGBHorizontalPixels = 160;
constexpr auto kGBVerticalPixels = 144;
constexpr auto kGBTotalPixels = kGBHorizontalPixels * kGBVerticalPixels;

using LCDPixels =
    std::array<std::array<GBColor, kGBVerticalPixels>, kGBHorizontalPixels>;

// ゲームボーイの画面を描画するクラス。
// 160x144（HiDPIの場合は擬似解像度換算）の整数倍のサイズのウインドウに描画する。
// 倍率はコンストラクタで指定する。
class Renderer {
 public:
  Renderer(int screen_scale = 1);
  ~Renderer();
  void RenderLCDPixels(const LCDPixels& pixels) const;

 private:
  int screen_scale_;  // ウインドウのサイズの拡大率
  int pixel_size_;  // ゲームボーイのLCDの1ピクセルを、1辺何ピクセルの正方形で表現するか
                    // このピクセル数はHiDPIかどうかに関係なくディスプレイの実際のピクセル数を指す
  SDL_Color colors_[kGBColorNum];  // enumから実際の色への対応
  SDL_Window* window_;
  SDL_Renderer* renderer_;
};

}  // namespace gbemu

#endif  // GBEMU_RENDERER_H_
