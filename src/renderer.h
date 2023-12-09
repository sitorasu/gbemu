#ifndef GBEMU_RENDERER_H_
#define GBEMU_RENDERER_H_

#include <SDL.h>

#include <array>

#include "ppu.h"

namespace gbemu {

// ゲームボーイの画面を描画するクラス。
// 160x144（HiDPIの場合は擬似解像度換算）の整数倍のサイズのウインドウに描画する。
// 倍率はコンストラクタで指定する。
class Renderer {
 public:
  Renderer(int screen_scale = 1);
  ~Renderer();
  void Render(const GbLcdPixelMatrix& pixels) const;
  bool vsync() { return vsync_; }

 private:
  int screen_scale_;  // ウインドウのサイズの拡大率
  int pixel_size_;  // ゲームボーイのLCDの1ピクセルを、1辺何ピクセルの正方形で表現するか
                    // このピクセル数はHiDPIかどうかに関係なくディスプレイの実際のピクセル数を指す
  bool vsync_;      // 垂直同期
  SDL_Window* window_;
  SDL_Renderer* renderer_;
};

}  // namespace gbemu

#endif  // GBEMU_RENDERER_H_
