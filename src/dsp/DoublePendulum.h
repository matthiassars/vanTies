#pragma once
#include <random>

class DoublePendulum {
public:
  static constexpr float TWOPI = 2.f * M_PI;
  static constexpr float TWOPI_INV = 1.f / TWOPI;
  static constexpr float RAND_MAX_INV = 1.f / RAND_MAX;

  float maxDTh = 0.f;
  float sampleTime = 0.f;
  float th1 = 0.f;
  float th2 = 0.f;
  float dTh1 = 0.f;
  float dTh2 = 0.f;
  float x1 = 0.f;
  float y1 = 0.f;
  float x2 = 0.f;
  float y2 = 0.f;
  float l = 1.f;
  float g = 9.8f;
  float cof = 0.f;
  float th1Prev = 0.f;
  float th2Prev = 0.f;
  float dTh1Prev = 0.f;
  float dTh2Prev = 0.f;
  bool th1Is0_ = false;
  bool th2Is0_ = false;

public:
  void setSampleRate(int sampleRate) {
    maxDTh = .5f * M_PI * sampleRate;
    sampleTime = 1.f / sampleRate;
  }

  float getX1() { return x1; }
  float getY1() { return y1; }
  float getX2Rel() { return x2; }
  float getY2Rel() { return y2; }
  float getX2Abs() { return x1 + x2; }
  float getY2Abs() { return y1 + y2; }
  bool th1Is0() { return th1Is0_; }
  bool th2Is0() { return th2Is0_; }
  float getG() { return g; }
  float getL() { return l; }
  float getCOF() { return cof; }

  void init(float th1 = M_PI, float th2 = M_PI);
  void setLength(float l) { this->l = l; }
  void setGravity(float g) { this->g = g; }
  void setCOF(float cof) { this->cof = cof ; }
  void process();
};
