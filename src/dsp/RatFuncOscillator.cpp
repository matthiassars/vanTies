#include "RatFuncOscillator.h"

using namespace std;

float RatFuncOscillator::phaseDistort1_1(float x, float c1) {
  float c2 = c1 * c1;
  float c3 = c2 * c1;

  return (2.f * (c3 - 2.f * c2 + c1) * x)
    / (x * (2.f * c3 - c1 * (x - 3.f)) + c2 * (2.f * x * x - 7.f * x + 1.f)
      + sqrtf(c1 * (4.f * c3 - 12.f * c2 + 13.f * c1 - 4.f))
      * (x - 1.f) * (c1 - x));
}

float RatFuncOscillator::phaseDistort2_1(float x, float c1) {
  float c2 = c1 * c1;
  float c3 = c2 * c1;

  return -(2.f * (c3 - 2.f * c2 + c1) * x)
    / (x * (c1 * (x - 3.f) - 2.f * c3) + c2 * (-2.f * x * x + 7.f * x - 1.f)
      + sqrtf(c1 * (4.f * c3 - 12.f * c2 + 13.f * c1 - 4.f))
      * (x - 1.f) * (c1 - x));
}

float RatFuncOscillator::primaryWaveFunction_1(float x) {
  x -= floorf(x);
  return min(max(
    (x * (2.f * x - 1.f) * (a - b) * (a - b))
    / (a * a * (2.f * SQRT2M1 * b * b - SQRT2M1 * b + x * (2.f * x - 1.f))
      + b * x * (-2.f * a * (2.f * SQRT2M1 * b + 2.f * x - M_SQRT2f)
        + b * (2.f * M_SQRT2f * x - 1.f)
        - SQRT2M1 * x)),
    -1.f), 1.f);
}

float RatFuncOscillator::primaryWaveFunction(float x) {
  x -= floorf(x);
  return (x < .5f) ?
    primaryWaveFunction_1(x) :
    -primaryWaveFunction_1(1.f - x);
}

// we don't need the following two inverse functions for audio output,
// only for putting the reference points on the oscilloscope widget

float RatFuncOscillator::phaseDistortInv1_1(float x, float c1) {
  float c2 = c1 * c1;
  float c3 = c2 * c1;
  float x2 = x * x;
  float A = sqrtf(c1 * (4.f * c3 - 12.f * c2 + 13.f * c1 - 4.f));

  return (-2.f * c3 * x + 2.f * c3 + 7.f * c2 * x - 4.f * c2 + A * c1 * x
    + A * x - 3.f * c1 * x + 2.f * c1)
    / (2.f * (2.f * c2 + A - c1) * x)
    - sqrtf((c3 - 2.f * c2 + c1)
      * (4.f * c3 * x2 - 4.f * c3 * x + 2.f * c3 - 16.f * c2 * x2
        + 14.f * c2 * x - 4.f * c2 - 2.f * A * c1 * x2 - 3.f * A * x2
        + 2.f * A * c1 * x + 2.f * A * x + 11.f * c1 * x2 - 6.f * c1 * x
        + 2.f * c1 - 2.f * x2))
    / (M_SQRT2f * (2.f * c2 + A - c1) * x);
}

float RatFuncOscillator::phaseDistortInv2_1(float x, float c1) {
  float c2 = c1 * c1;
  float c3 = c2 * c1;
  float x2 = x * x;
  float A = sqrtf(c1 * (4.f * c3 - 12.f * c2 + 13.f * c1 - 4.f));

  return -(-2.f * c3 * x + 2.f * c3 + 7.f * c2 * x - 4.f * c2 - A * c1 * x
    - A * x - 3.f * c1 * x + 2.f * c1)
    / (2.f * (-2.f * c2 + A + c1) * x)
    + sqrtf((c3 - 2.f * c2 + c1)
      * (4.f * c3 * x2 - 4.f * c3 * x + 2.f * c3 - 16.f * c2 * x2
        + 14.f * c2 * x - 4.f * c2 + 2.f * A * c1 * x2 + 3.f * A * x2
        - 2.f * A * c1 * x - 2.f * A * x + 11.f * c1 * x2 - 6.f * c1 * x
        + 2.f * c1 - 2 * x2))
    / (M_SQRT2f * (-2.f * c2 + A + c1) * x);
}

float RatFuncOscillator::phaseDistort1(float x) {
  x -= floorf(x);
  return (c > .5f) ?
    phaseDistort1_1(x, c) :
    -phaseDistort2_1(1.f - x, 1.f - c);
}

float RatFuncOscillator::phaseDistort2(float x) {
  x -= floorf(x);
  return (c > .5f) ?
    phaseDistort2_1(x, c) :
    -phaseDistort1_1(1.f - x, 1.f - c);
}

float RatFuncOscillator::phaseDistortInv1(float x) {
  float y;
  if (c > .5f) {
    x -= floorf(x);
    y = phaseDistortInv2_1(x, c);
  } else {
    x = -x;
    x -= floorf(x);
    y = -phaseDistortInv1_1(x, 1.f - c);
  }
  y -= floorf(y);
  return y;
}

float RatFuncOscillator::phaseDistortInv2(float x) {
  float y;
  if (c > .5f) {
    x -= floorf(x);
    y = phaseDistortInv1_1(x, c);
  } else {
    x = -x;
    x -= floorf(x);
    y = -phaseDistortInv2_1(x, 1.f - c);
  }
  y -= floorf(y);
  return y;
}

// set the paramaters a, b, and c as values between 0. and 1.
void RatFuncOscillator::setParams(float a, float b, float c) {
  a = min(max(a, 0.f), 1.f);
  b = min(max(b, 0.f), 1.f);
  c = min(max(c, 0.f), 1.f);

  // map the parameters in such a way that the worst aliasing is avoided
  // (lots of more or less educated guessing is going on here)

  // exclude values of c around 0 and 1
  float d = min(8.f * abs((float)dPh[0]), .5f);
  c = min(max(c, d), 1.f - d);

  // range of a (0, .5)
  a *= .5f;
  // exclude values of a around 0 and .5
  d = min(4.f * abs((float)dPh[0] / min(c, 1.f - c)), .25f);
  a = min(max(a, d), .5f - .5f * d);

  // range of b: (a, .5)
  b = a + (.5f - a) * b;
  // exclude values of b around a and .5
  d = min(4.f * abs((float)dPh[0] / ((a - .5f) * min(c, 1.f - c))),
    .25f - .5f * a);
  b = min(max(b, a + d), .5f - d);

  this->a = a;
  this->b = b;
  this->c = c;
}

void RatFuncOscillator::process() {
  wave[0] = waveFunction1(ph[0]);
  wave[1] = waveFunction2(ph[0]);

  incrementPhases();
}
