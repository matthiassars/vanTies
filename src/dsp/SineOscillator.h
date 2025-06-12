#pragma once
#include "Oscillator.h"

// and a class for the oscillator for the fundamental
class SineOscillator : public Oscillator<> {
public:
  void process() override {
    wave[0] = abs(dPh[0] < .5) ? sinf(TWOPI * ph[0]) : 0.f;
    incrementPhases();
  }
};
