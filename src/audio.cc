#include "audio.h"

#include <SDL.h>

#include "utils.h"

using namespace gbemu;

namespace {

void AudioCallback(void *userdata, Uint8 *_stream, int _length) {
  Audio *audio = (Audio *)userdata;
  audio->AudioCallback(_stream, _length);
}

};  // namespace

Audio::Audio() {
  SDL_AudioSpec desired;

  desired.freq = kFreqency;
  desired.format = AUDIO_S16SYS;
  desired.channels = 2;
  desired.samples = 2048;
  desired.callback = ::AudioCallback;
  desired.userdata = this;

  SDL_AudioSpec obtained;

  SDL_OpenAudio(&desired, &obtained);

  SDL_PauseAudio(0);
}

Audio::~Audio() { SDL_CloseAudio(); }

void Audio::PushSample(double left, double right) {
  // 音が遅れすぎている（サンプルが過剰に溜まっている）場合、
  // 消費されるのを待つ
  while (samples_.size() > kMaxBufferSize) {
    SDL_Delay(1);
  }

  SDL_LockAudio();
  samples_.push_back(left);
  samples_.push_back(right);
  SDL_UnlockAudio();
}

namespace {

// キューの先頭要素をキューから削除したうえで返す。
// キューが空の場合は0を返す。
double PopSample(std::deque<double> &buf) {
  double ret = 0.0;
  if (!buf.empty()) {
    SDL_LockAudio();
    ret = buf.front();
    buf.pop_front();
    SDL_UnlockAudio();
  }
  return ret;
}

}  // namespace

void Audio::AudioCallback(Uint8 *_stream, int _length) {
  Sint16 *stream = (Sint16 *)_stream;
  int length = _length / 2;
  ASSERT(length % 2 == 0,
         "The number of left/right samples required are mismatched.");

  for (int i = 0; i < length; i++) {
    stream[i] = kAmplitude * PopSample(samples_);
  }
}
