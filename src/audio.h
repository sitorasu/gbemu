#ifndef GBEMU_AUDIO_H_
#define GBEMU_AUDIO_H_

namespace gbemu {

class Audio {
 public:
  Audio();
  ~Audio();

 private:
  static constexpr int kFreqency = 44100;
  static constexpr int kAmplitude = 3000;
};

}  // namespace gbemu

#endif  // GBEMU_AUDIO_H_
