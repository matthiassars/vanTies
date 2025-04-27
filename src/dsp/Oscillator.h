#pragma once
#include <iostream>
#include <cmath>
#include "rack.hpp"

// an abstract class for oscillators
template <size_t phasors = 1, size_t waveforms = 1>
class Oscillator {
protected:
  static constexpr float TWOPI = 2.f * M_PI;

  double dPhase[phasors] = {};
  double phase[phasors] = {};
  float wave[waveforms] = {};

  void incrementPhases() {
    for (size_t i = 0; i < phasors; i++) {
      phase[i] += dPhase[i];
      phase[i] -= floorf(phase[i]);
    }
  }

public:
  void setFreq(float freq, size_t i = 0) {
    dPhase[i] = freq * APP->engine->getSampleTime();
  }

  float getFreq(size_t i = 0) {
    if (i > phasors)
      return 0.f;
    return dPhase[i] * APP->engine->getSampleRate();
  }

  float getWave(size_t i = 0) {
    if (i > waveforms)
      return 0.f;
    return wave[i];
  }

  virtual void process() = 0;

  void reset() {
    for (size_t i = 0; i < phasors; i++)
      phase[i] = 0.;
    for (size_t i = 0; i < waveforms; i++)
      wave[i] = 0.f;
  }
};
