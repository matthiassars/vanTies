#pragma once
#include <iostream>
#include "rack.hpp"
#include "Oscillator.h"

// and a class for the oscillator for the fundamental
class SineOscillator : public Oscillator<> {
public:
  void process() override {
    wave[0] = sinf(TWOPI * phase[0]);
    incrementPhases();
  }
};
