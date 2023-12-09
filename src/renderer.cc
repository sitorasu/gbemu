#include "renderer.h"

#include <SDL.h>

#include <array>
#include <utility>

#include "utils.h"

using namespace gbemu;

namespace {

SDL_Color LcdColorToSdlColor(lcd::GbLcdColor color) {
  switch (color) {
    case lcd::kWhite:
      return {232, 232, 232, 255};
    case lcd::kLightGray:
      return {160, 160, 160, 255};
    case lcd::kDarkGray:
      return {88, 88, 88, 255};
    case lcd::kBlack:
      return {16, 16, 16, 255};
    default:
      UNREACHABLE("Invalid LCD Color: %d", color);
  }
}

};  // namespace

Renderer::Renderer(int screen_scale)
    : screen_scale_(screen_scale <= 0 ? 1 : screen_scale), vsync_(false) {
  int screen_width = lcd::kWidth * screen_scale_;
  int screen_height = lcd::kHeight * screen_scale_;
  window_ = SDL_CreateWindow(
      "GBEMU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width,
      screen_height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
  if (window_ == nullptr) {
    Error("SDL_CreateWindow Error: %s", SDL_GetError());
  }

  // ディスプレイのリフレッシュレートが60Hzなら垂直同期オン
  int disp = SDL_GetWindowDisplayIndex(window_);
  SDL_DisplayMode mode;
  SDL_GetCurrentDisplayMode(disp, &mode);
  Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
  if (mode.refresh_rate == 60) {
    renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    vsync_ = true;
  }

  renderer_ = SDL_CreateRenderer(window_, -1, renderer_flags);
  if (renderer_ == nullptr) {
    Error("SDL_CreateRenderer Error: %s", SDL_GetError());
  }

  // 描画可能領域の実際のピクセル数を取得する。
  // ※HiDPI環境ではSDL_CreateWindowで指定したサイズは擬似解像度換算。
  int drawable_width, drawable_height;
  SDL_GetRendererOutputSize(renderer_, &drawable_width, &drawable_height);

  // 描画可能領域を160x144の格子に分割するには
  // 格子の1辺を何ピクセルから構成すればよいか調べる。
  // 格子に分割できない、あるいは格子が正方形とならない場合はエラーとする。
  if ((drawable_width % lcd::kWidth) != 0 ||
      (drawable_height % lcd::kHeight) != 0 ||
      (drawable_width / lcd::kWidth) != (drawable_height / lcd::kHeight)) {
    Error("Not supported scaling rate");
  }
  pixel_size_ = drawable_width / lcd::kWidth;
}

Renderer::~Renderer() {
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
}

void Renderer::Render(const GbLcdPixelMatrix& buffer) const {
  SDL_SetRenderDrawColor(renderer_, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderClear(renderer_);
  for (int i = 0; i < lcd::kHeight; i++) {
    for (int j = 0; j < lcd::kWidth; j++) {
      SDL_Color color = LcdColorToSdlColor(buffer[i][j]);
      SDL_Rect rect;
      rect.x = j * pixel_size_;
      rect.y = i * pixel_size_;
      rect.w = pixel_size_;
      rect.h = pixel_size_;
      SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
      SDL_RenderFillRect(renderer_, &rect);
    }
  }
  SDL_RenderPresent(renderer_);
}
