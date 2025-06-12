#include "DoublePendulum.h"
#include <iostream>

using namespace std;

void DoublePendulum::init(float th1, float th2) {
  this->th1 = th1;
  this->th2 = th2;
  this->th1 += 1.e-2 * (rand() * RAND_MAX_INV - .5f);
  this->th2 += 1.e-2 * (rand() * RAND_MAX_INV - .5f);
  dTh1 = 0.f;
  dTh2 = 0.f;

  this->l = l;
  this->g = g;
}

void DoublePendulum::process() {
  float thDiff = th1 - th2;
  float dTh1Sq = dTh1 * dTh1;
  float dTh2Sq = dTh2 * dTh2;
  float sinTh1 = sinf(th1);
  float cosTh1 = cosf(th1);
  float sinThDiff = sinf(thDiff);
  float cosThDiff = cosf(thDiff);
  float A = 1.f / (l * (3.f - cosf(2.f * thDiff)));

  x1 = sinTh1;
  y1 = -cosTh1;
  x2 = sinf(th2);
  y2 = -cosf(th2);

  float ddTh1 = A * (-3.f * g * sinTh1 - g * sinf(th1 - 2.f * th2)
    - 2.f * sinThDiff * l * (dTh2Sq + dTh1Sq * cosThDiff))
    - cof * dTh1 * sampleTime;
  float ddTh2 = 2.f * A * sinThDiff
    * (2.f * dTh1Sq * l + 2.f * g * cosTh1 + dTh2Sq * l * cosThDiff)
    - cof * dTh2 * sampleTime;

  th1Prev = th1;
  th2Prev = th2;
  dTh1Prev = dTh1;
  dTh2Prev = dTh2;
  dTh1 += ddTh1 * sampleTime;
  dTh2 += ddTh2 * sampleTime;
  if (abs(dTh1) > maxDTh)
    dTh1 = 0.f;
  if (abs(dTh2) > maxDTh)
    dTh2 = 0.f;
  th1 += dTh1 * sampleTime;
  th2 += dTh2 * sampleTime;
  th1 -= floorf(th1 * TWOPI_INV) * TWOPI;
  th2 -= floorf(th2 * TWOPI_INV) * TWOPI;

  if (abs(th1 - th1Prev) > 6.25f) {
    th1Is0_ = true;
  } else if (((dTh1Prev < 0.f) ^ (dTh1 < 0.f))
    || ((th1Prev < M_PI) ^ (th1 < M_PI)))
    th1Is0_ = false;
  if (abs(th2 - th2Prev) > 6.f)
    th2Is0_ = true;
  else if (((dTh2Prev < 0.f) ^ (dTh2 < 0.f))
    || ((th2Prev < M_PI) ^ (th2 < M_PI)))
    th2Is0_ = false;
}

