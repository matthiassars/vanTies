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

    float thDiff = th1 - th2;

    float dTh1Sq = dTh1 * dTh1;
    float dTh2Sq = dTh2 * dTh2;
    float sinTh1Th2 = sinf(thDiff);
    float cosTh1Th2 = cosf(thDiff);
    float A = 1.f / (l * (3.f - cosf(2.f * (thDiff))));
  
    float sinTH1 = sinf(th1);
    float cosTH1 = cosf(th1);


    float ddTh1 = A * (-3.f * g * sinTH1 - g * sinf(th1 - 2.f * th2)
      - 2.f * sinTh1Th2 * l * (dTh2Sq + dTh1Sq * cosTh1Th2));
    float ddTh2 = 2.f * A * sinTh1Th2
      * (2 * dTh1Sq * l + 2.f * g * cosTH1 + dTh2Sq * l * cosTh1Th2);
  
    float dt = APP->engine->getSampleTime();
    dTh1 += ddTh1 * dt;
    dTh2 += ddTh2 * dt;
  
    th1 += dTh1 * dt;
    th2 += dTh2 * dt;
  
    x1 = sinTH1;
    y1 = -cosTH1;
    x2 = sinf(th2);
    y2 = -cosf(th2);
  }
