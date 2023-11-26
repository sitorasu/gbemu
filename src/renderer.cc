#include "renderer.h"

#include <SDL.h>

#include <array>
#include <utility>

#include "utils.h"

using namespace gbemu;

Renderer::Renderer(int screen_scale)
    : screen_scale_(screen_scale <= 0 ? 1 : screen_scale) {
  int screen_width = kGBHorizontalPixels * screen_scale_;
  int screen_height = kGBVerticalPixels * screen_scale_;
  window_ = SDL_CreateWindow(
      "GBEMU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width,
      screen_height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
  if (window_ == nullptr) {
    Error("SDL_CreateWindow Error: %s", SDL_GetError());
  }

  renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
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
  if ((drawable_width % kGBHorizontalPixels) != 0 ||
      (drawable_height % kGBVerticalPixels) != 0 ||
      (drawable_width / kGBHorizontalPixels) !=
          (drawable_height / kGBVerticalPixels)) {
    Error("Not supported scaling rate");
  }
  pixel_size_ = drawable_width / kGBHorizontalPixels;

  colors_[kWhite] = {232, 232, 232, 255};
  colors_[kLightGray] = {160, 160, 160, 255};
  colors_[kDarkGray] = {88, 88, 88, 255};
  colors_[kBlack] = {16, 16, 16, 255};
}

Renderer::~Renderer() {
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
}

void Renderer::RenderLCDPixels(const LCDPixels& pixels) const {
  SDL_SetRenderDrawColor(renderer_, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderClear(renderer_);
  for (int i = 0; i < kGBHorizontalPixels; i++) {
    for (int j = 0; j < kGBVerticalPixels; j++) {
      SDL_Color color = colors_[pixels[i][j]];
      SDL_Rect rect;
      rect.x = i * pixel_size_;
      rect.y = j * pixel_size_;
      rect.w = pixel_size_;
      rect.h = pixel_size_;
      SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
      SDL_RenderFillRect(renderer_, &rect);
    }
  }
  SDL_RenderPresent(renderer_);
}
