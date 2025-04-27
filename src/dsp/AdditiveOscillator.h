#pragma once
#include <iostream>
#include "rack.hpp"
#include "Oscillator.h"
#include "SpectrumStereo.h"

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

  void init(SpectrumStereo* spec) { this->spec = spec; }

  static float quantStretch(float stretch, StretchQuant stretchQuant);

  void setFreq(float freq);

  void setStretch(float stretch, StretchQuant stretchQuant) {
    this->stretch = quantStretch(stretch, stretchQuant);
  }

  float getWave(size_t i);

  float getStretch() { return stretch; }

  void process() override;

private:
  float stretch;

  SpectrumStereo* spec = nullptr;
};
