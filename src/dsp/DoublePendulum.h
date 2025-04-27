#pragma once
#include <iostream>
#include "rack.hpp"

class DoublePendulum {
private:
  static constexpr float TWOPI = 2.f * M_PI;
  static constexpr float TWOPI_INV = 1.f / TWOPI;
  static  constexpr float RAND_MAX_INV = 1.f / RAND_MAX;

  float th1 = 0.f;
  float th2 = 0.f;
  float dTh1 = 0.f;
  float dTh2 = 0.f;
  float x1;
  float y1;
  float x2;
  float y2;
  float l = 1.f;
  float g = 9.8f;

public:
  float getX1() { return x1; }
  float getY1() { return y1; }
  float getX2Rel() { return x2; }
  float getY2Rel() { return y2; }
  float getX2Abs() { return x1 + x2; }
  float getY2Abs() { return y1 + y2; }

  void init(float th1, float th2, float l, float g);
  void process();
};
