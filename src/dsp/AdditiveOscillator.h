#pragma once
#include "Oscillator.h"
#include "Spectrum.h"

// a class for the additive oscillator
// We need 3 phasors. In the "process" method we'll see why.
// 2 waveforms for stereo
class AdditiveOscillator : public Oscillator<3, 2> {
public:
  enum StretchQuant {
    CONTINUOUS,
    CONSONANTS,
    HARMONICS
  };

  void init(int sampleRate, Spectrum* spec) {
    setSampleRate(sampleRate);
    this->spec = spec;
  }

  static float quantStretch(float stretch, StretchQuant stretchQuant);

  inline void setFreq(float freq) {
    dPh[0] = freq * sampleTime;
    dPh[2] = stretch * dPh[0];
    dPh[1] = dPh[0] + dPh[2];
  }

  inline void setStretch(float stretch, StretchQuant stretchQuant) {
    this->stretch = quantStretch(stretch, stretchQuant);
  }

  float getStretch() { return stretch; }

  void process() override;

private:
  float stretch;

  Spectrum* spec = nullptr;
};
