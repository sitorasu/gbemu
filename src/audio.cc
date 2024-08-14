#include "audio.h"

#include <SDL.h>

using namespace gbemu;

namespace {

void AudioCallback(void *userdata, Uint8 *stream, int length);

};

Audio::Audio() {
  SDL_AudioSpec desired;

  desired.freq = kFreqency;
  desired.format = AUDIO_S16SYS;
  desired.channels = 2;
  desired.samples = 2048;
  desired.callback = AudioCallback;
  desired.userdata = this;

  SDL_AudioSpec obtained;

  SDL_OpenAudio(&desired, &obtained);

  SDL_PauseAudio(0);
}

Audio::~Audio() { SDL_CloseAudio(); }
