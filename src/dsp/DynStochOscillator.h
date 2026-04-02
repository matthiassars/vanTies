#pragma once
#include "Oscillator.h"
#include <iostream>
#include <cmath>
#include <random>
#include <list>
#include <algorithm>

class DynStochOscillator : public Oscillator<> {
public:
  enum RefWaveform { SINE, ZERO };

  DynStochOscillator();

  static constexpr float RAND_MAX_INV = 1.f / RAND_MAX;
  static constexpr float EPS = 1e-6f;

  void reset(bool randomize);

  void setSegments(int segments);

  void setRandAmt_t(float randAmt) { randAmt_t = segmentsInv * randAmt; }

  void setRandAmt_a(float randAmt) { randAmt_a = 2.f * randAmt; }

  void setDestruct_t(float destruct) { destruct_t = destruct; }

  void setDestruct_a(float destruct) { destruct_a = destruct; }

  void setRefWaveform(RefWaveform refWaveform) { this->refWaveform = refWaveform; }

  inline int getSegments() { return segments; }

  void process() override;

  struct Breakpoint {
    Breakpoint(float t, float a) {
      this->t = t;
      this->a = a;
    }

    float t;
    float a;

    bool operator<(const Breakpoint other) const { return t < other.t; }
  };

  std::list<Breakpoint> breakpoints;

private:
  float sampleTime = 0.f;
  int segments = 1;
  float segmentsInv = .5f;
  std::list<Breakpoint>::iterator nextBp = breakpoints.begin();
  float randAmt_t = 0.f;
  float randAmt_a = 0.f;
  float destruct_t = 0.f;
  float destruct_a = 0.f;
  float slope = 0.f;
  float intercept = 0.f;
  RefWaveform refWaveform = SINE;

  inline float rand(float x1, float x2) {
    return  x1 + (x2 - x1) * RAND_MAX_INV * std::rand();
  }

  inline float mirror(float x, float min, float max) {
    if (max <= min + EPS)
      return std::clamp(x, min, max);

    float diff2 = 2.f * (max - min);
    while (x < min)
      x += diff2;
    float max2 = 2.f * max - min;
    while (x >= max2)
      x -= diff2;
    return (x <= max) ? x : 2.f * max - x;
  }

  void insertBreakpoint(bool random = true);

  void eraseBreakpoint();

  void computeSlope();

  void wiggle(std::list<Breakpoint>::iterator i);

  void wiggleEdgeBp();

  float refFunction(float x);

  float getLength() { return breakpoints.back().t; }
};
