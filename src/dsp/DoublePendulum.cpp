#include "DoublePendulum.h"

using namespace std;

void DoublePendulum::init(float th1, float th2, float l, float g) {
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
  th1 -= floor(th1 * TWOPI_INV) * TWOPI;
  th2 -= floor(th2 * TWOPI_INV) * TWOPI;

  float dTh1Sq = dTh1 * dTh1;
  float dTh2Sq = dTh2 * dTh2;
  float sinTh1Th2 = sinf(th1 - th2);
  float cosTh1Th2 = cosf(th1 - th2);
  float A = 1.f / (l * (3.f - cosf(2.f * (th1 - th2))));

  float ddTh1 = A * (-3.f * g * sinf(th1) - g * sinf(th1 - 2.f * th2)
    - 2.f * sinTh1Th2 * l * (dTh2Sq + dTh1Sq * cosTh1Th2));
  float ddTh2 = 2.f * A * sinTh1Th2
    * (2 * dTh1Sq * l + 2.f * g * cosf(th1) + dTh2Sq * l * cosTh1Th2);

  dTh1 += ddTh1 * APP->engine->getSampleTime();
  dTh2 += ddTh2 * APP->engine->getSampleTime();

  th1 += dTh1 * APP->engine->getSampleTime();
  th2 += dTh2 * APP->engine->getSampleTime();

  x1 = sinf(th1);
  y1 = -cosf(th1);
  x2 = sinf(th2);
  y2 = -cosf(th2);
}
