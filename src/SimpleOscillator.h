#pragma once
#include "Oscillator.h"

// and a class for the oscillator for the fundamental
class SimpleOscillator : public Oscillator {
private:
  bool waveSquare;

public:
  void setWaveSquare(bool waveSquare) {
    this->waveSquare = waveSquare;
  }

  void process() override {
    incrementPhase();

    if (waveSquare) {
      if (phase < .5f)
        wave = 1.f;
      else
        wave = -1.f;
    } else
      wave = sin2piBhaskara(phase);
  }
};
