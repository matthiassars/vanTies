#pragma once
#include "Oscillator.h"

using namespace std;

class RatFuncOscillator : public Oscillator {
private:
  float wave2;

  float a;
  float b;
  float c;

  static constexpr float SQRT2M1 = M_SQRT2 - 1.f;

  inline float phaseDistort1_1(float x, float c1) {
    float c2 = c1 * c1;
    float c3 = c2 * c1;

    return (2.f * (c3 - 2.f * c2 + c1) * x)
      / (x * (2.f * c3 - c1 * (x - 3.f)) + c2 * (2.f * x * x - 7.f * x + 1.f)
        + sqrtf(c1 * (4.f * c3 - 12.f * c2 + 13.f * c1 - 4.f))
        * (x - 1.f) * (c1 - x));
  }

  inline float phaseDistort2_1(float x, float c1) {
    float c2 = c1 * c1;
    float c3 = c2 * c1;

    return -(2.f * (c3 - 2.f * c2 + c1) * x)
      / (x * (c1 * (x - 3.f) - 2.f * c3) + c2 * (-2.f * x * x + 7.f * x - 1.f)
        + sqrt(c1 * (4.f * c3 - 12.f * c2 + 13.f * c1 - 4.f))
        * (x - 1.f) * (c1 - x));
  }

  inline float primaryWaveFunction_1(float x) {
    x -= floor(x);
    return clamp(
      (x * (2.f * x - 1.f) * (a - b) * (a - b))
      / (a * a * (2.f * SQRT2M1 * b * b - SQRT2M1 * b + x * (2.f * x - 1.f))
        + b * x * (-2.f * a * (2.f * SQRT2M1 * b + 2.f * x - M_SQRT2)
          + b * (2.f * M_SQRT2 * x - 1.f)
          - SQRT2M1 * x)),
      -1.f, 1.f);
  }

  inline float primaryWaveFunction(float x) {
    x -= floorf(x);
    if (x < .5f)
      return primaryWaveFunction_1(x);
    return -primaryWaveFunction_1(1.f - x);
  }

  // we dont' need the following two inverse functions for audio output,
  // only for puting the reference points on the oscilloscope widget

  inline float phaseDistortInv1_1(float x, float c1) {
    float c2 = c1 * c1;
    float c3 = c2 * c1;
    float x2 = x * x;
    float A = sqrtf(c1 * (4.f * c3 - 12.f * c2 + 13.f * c1 - 4.f));

    return (-2.f * c3 * x + 2.f * c3 + 7.f * c2 * x - 4.f * c2 + A * c1 * x
      + A * x - 3.f * c1 * x + 2.f * c1)
      / (2.f * (2.f * c2 + A - c1) * x)
      - sqrt((c3 - 2.f * c2 + c1)
        * (4.f * c3 * x2 - 4.f * c3 * x + 2.f * c3 - 16.f * c2 * x2
          + 14.f * c2 * x - 4.f * c2 - 2.f * A * c1 * x2 - 3.f * A * x2
          + 2.f * A * c1 * x + 2.f * A * x + 11.f * c1 * x2 - 6.f * c1 * x
          + 2.f * c1 - 2.f * x2))
      / (M_SQRT2 * (2.f * c2 + A - c1) * x);
  }

  inline float phaseDistortInv2_1(float x, float c1) {
    float c2 = c1 * c1;
    float c3 = c2 * c1;
    float x2 = x * x;
    float A = sqrtf(c1 * (4.f * c3 - 12.f * c2 + 13.f * c1 - 4.f));

    return -(-2.f * c3 * x + 2.f * c3 + 7.f * c2 * x - 4.f * c2 - A * c1 * x
      - A * x - 3.f * c1 * x + 2.f * c1)
      / (2.f * (-2.f * c2 + A + c1) * x)
      + sqrt((c3 - 2.f * c2 + c1)
        * (4.f * c3 * x2 - 4.f * c3 * x + 2.f * c3 - 16.f * c2 * x2
          + 14.f * c2 * x - 4.f * c2 + 2.f * A * c1 * x2 + 3.f * A * x2
          - 2.f * A * c1 * x - 2.f * A * x + 11.f * c1 * x2 - 6.f * c1 * x
          + 2.f * c1 - 2 * x2))
      / (M_SQRT2 * (-2.f * c2 + A + c1) * x);
  }

public:
  void setA(float a) { this->a = a; }
  void setB(float b) { this->b = b; }
  void setC(float c) { this->c = c; }

  float getWave2() { return wave2; }

  float getA() { return a; }
  float getB() { return b; }
  float getC() { return c; }

  inline float phaseDistort1(float x) {
    x -= floor(x);
    if (c > .5f)
      return phaseDistort1_1(x, c);
    if (c < .5f)
      return -phaseDistort2_1(1.f - x, 1.f - c);
    return x;
  }

  inline float phaseDistort2(float x) {
    x -= floor(x);
    if (c > .5f)
      return phaseDistort2_1(x, c);
    if (c < .5f)
      return -phaseDistort1_1(1.f - x, 1.f - c);
    return x;
  }

  inline float waveFunction1(float x) {
    return primaryWaveFunction(phaseDistort1(x));
  }

  inline float waveFunction2(float x) {
    return primaryWaveFunction(phaseDistort2(x));
  }

  inline float phaseDistortInv1(float x) {
    float y;
    if (c > .5f) {
      x -= floor(x);
      y = phaseDistortInv2_1(x, c);
    } else if (c == .5f)
      y = x;
    else {
      x = -x;
      x -= floor(x);
      y = -phaseDistortInv1_1(x, 1.f - c);
    }
    y -= floor(y);
    return y;
  }

  inline float phaseDistortInv2(float x) {
    float y;
    if (c > .5f) {
      x -= floor(x);
      y = phaseDistortInv1_1(x, c);
    } else if (c == .5f)
      y = x;
    else {
      x = -x;
      x -= floor(x);
      y = -phaseDistortInv2_1(x, 1.f - c);
    }
    y -= floor(y);
    return y;
  }

  void process() override {
    incrementPhase();

    wave = waveFunction1(phase);
    wave2 = waveFunction2(phase);
  }
};
