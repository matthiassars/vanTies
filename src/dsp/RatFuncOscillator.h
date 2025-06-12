#pragma once
#include <algorithm>
#include "Oscillator.h"

class RatFuncOscillator : public Oscillator<1, 2> {
private:
  float wave2;

  float a;
  float b;
  float c;

  static constexpr float SQRT2M1 = M_SQRT2f - 1.f;

  float phaseDistort1_1(float x, float c1);
  float phaseDistort2_1(float x, float c1);
  float primaryWaveFunction_1(float x);
  float primaryWaveFunction(float x);
  // we don't need the following two inverse functions for audio output,
  // only for puting the reference points on the oscilloscope widget
  float phaseDistortInv1_1(float x, float c1);
  float phaseDistortInv2_1(float x, float c1);

public:
  // set the paramaters a, b, and c as values between 0. and 1.
  void setParams(float a, float b, float c);

  float getA() { return a; }
  float getB() { return b; }
  float getC() { return c; }

  float phaseDistort1(float x);
  float phaseDistort2(float x);
  float waveFunction1(float x) {
    return primaryWaveFunction(phaseDistort1(x));
  }
  float waveFunction2(float x) {
    return primaryWaveFunction(phaseDistort2(x));
  }
  float phaseDistortInv1(float x);
  float phaseDistortInv2(float x);
  void process() override;
};
