#pragma once
#include <cmath>

using namespace std;

// and an abstract class for oscillators
class Oscillator {
protected:
  double phase = 0.;
  double dPhase = 0.;
  float wave = 0.f;

  void incrementPhase() {
    phase += dPhase;
    phase -= floorf(phase);
  }

  inline float sin2piBhaskara(float x) {
    // Bhaskara I's sine approximation formula
    // for the function sin(2*pi*x)
    x -= floorf(x);
    if (x < .5f) {
      float A = x * (.5f - x);
      return 4.f * A / (.3125f - A); // 5/16 = .3125
    }
    float A = (x - .5f) * (x - 1.f);
    return 4.f * A / (.3125f + A);
  }

public:
  void setFreq(float freq) { dPhase = freq * APP->engine->getSampleTime(); }
  float getFreq() { return dPhase * APP->engine->getSampleRate(); }
  float getWave() { return wave; }

  virtual void process() = 0;

  void reset() {
    phase = 0.;
    wave = 0.f;
  }
};
