#ifndef GBEMU_AUDIO_H_
#define GBEMU_AUDIO_H_

#include <SDL.h>

#include <deque>

namespace gbemu {

// 外からサンプルの供給を受けて音を鳴らすクラス
class Audio {
 public:
  Audio();
  ~Audio();

  void PushSample(double left, double right);
  void AudioCallback(Uint8 *_stream, int _length);

 private:
  static constexpr int kFreqency = 44100;
  static constexpr int kAmplitude = 3000;
  static constexpr int kMaxBufferSize = 8192;

  // 左右の音のサンプルを交互に格納するキュー
  std::deque<double> samples_;
};

}  // namespace gbemu

#endif  // GBEMU_AUDIO_H_
