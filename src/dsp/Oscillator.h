#pragma once
#include <cmath>

// an abstract class for oscillators
template <int phasors = 1, int waveforms = 1>
class Oscillator {
public:
  static constexpr float TWOPI = 2.f * M_PI;

  int sampleRate = 0;
  float sampleTime = 0.f;

  double dPh[phasors] = {};
  double ph[phasors] = {};
  float wave[waveforms] = {};

  void incrementPhases() {
    for (int i = 0; i < phasors; i++) {
      ph[i] += dPh[i];
      ph[i] -= floorf(ph[i]);
    }
  }

public:
  void setSampleRate(int sampleRate) {
    this->sampleRate = sampleRate;
    sampleTime = 1.f / sampleRate;
  }

  inline void setFreq(float freq, int i = 0) {
    dPh[i] = freq * sampleTime;
  }

  inline float getFreq(int i = 0) {
    return dPh[i] * sampleRate;
  }

  inline float getWave(int i = 0) { return wave[i]; }

  virtual void process() = 0;

  inline void reset() {
    for (int i = 0; i < phasors; i++)
      ph[i] = 0.;
    for (int i = 0; i < waveforms; i++)
      wave[i] = 0.f;
  }
};
