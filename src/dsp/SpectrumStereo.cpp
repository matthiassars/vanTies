#include "SpectrumStereo.h"

using namespace std;

void SpectrumStereo::init(int oscs, CvBuffer* buf, bool* partialLeft) {
  oscs = max(oscs, 0);
  this->oscs = oscs;
  amps_tmp = new float[oscs];
  amps = new float[oscs];
  ampsR = new float[oscs];
  ampsSmooth = new float[oscs];
  ampsRSmooth = new float[oscs];
  set0();
  this->buf = buf;
  this->partialLeft = partialLeft;
}

SpectrumStereo::~SpectrumStereo() {
  delete ampsR;
  delete ampsRSmooth;
}

void SpectrumStereo::set0() {
  for (int i = 0; i < oscs; i++) {
    amps_tmp[i] = 0.f;
    amps[i] = 0.f;
    ampsR[i] = 0.f;
    ampsSmooth[i] = 0.f;
    ampsRSmooth[i] = 0.f;
  }
}

float SpectrumStereo::getAmpR(int i) {
  if (i < 0 || i >= oscs || zeroAmp || stereoMode == StereoMode::MONO)
    return 0.f;
  return ampsRSmooth[i];
}

void SpectrumStereo::process() {
  process_tmp();

  if (stereoMode == MONO) { // mono mode
    for (int i = 0; i < oscs; i++)
      amps[i] = amps_tmp[i];
  } else if (stereoMode == SOFT_PAN) { // soft panned mode
    amps[0] = amps_tmp[0];
    ampsR[0] = amps_tmp[0];
    float l = 0.f;
    if (lowest > 2.f)
      l = lowest - 2.f;
    for (int i = 1; i < oscs; i++) {
      if (partialLeft[i - 1] ^ flip) {
        amps[i] = amps_tmp[i];
        if (i + 1 > l)
          ampsR[i] = amps_tmp[i] / sqrtf(i + 1.f - l);
        else
          ampsR[i] = 0.f;
      } else {
        ampsR[i] = amps_tmp[i];
        if (i + 1 > l)
          amps[i] = amps_tmp[i] / sqrtf(i + 1.f - l);
        else
          amps[i] = 0.f;
      }
    }
  } else { // hard panned mode
    amps[0] = amps_tmp[0];
    ampsR[0] = amps_tmp[0];
    for (int i = 1; i < oscs; i++) {
      if (partialLeft[i - 1] ^ flip) {
        amps[i] = amps_tmp[i];
        ampsR[i] = 0.f;
      } else {
        amps[i] = 0.f;
        ampsR[i] = amps_tmp[i];
      }
    }
  }
}

void SpectrumStereo::smoothen() {
  for (int i = 0; i < oscs; i++) {
    ampsSmooth[i] += blockRatio * (amps[i] - ampsSmooth[i]);
    if (stereoMode != StereoMode::MONO)
      ampsRSmooth[i] += blockRatio * (ampsR[i] - ampsRSmooth[i]);
  }
}

float SpectrumStereo::getAbsAmp(int i) {
  return abs(max(getAmp(i), getAmpR(i)));
}

int SpectrumStereo::distDown(int i) {
  int j = i;
  do
    j--;
  while (j >= lowestI && getAbsAmp(j) < 1.e-3f * getAbsAmp(i));
  // return -1 for "infinity"
  if (j < lowestI - 1)
    return -1;
  return i - j;
}

int SpectrumStereo::distUp(int i) {
  int j = i;
  do
    j++;
  while (j <= highestI && getAbsAmp(j) < 1.e-3f * getAbsAmp(i));
  if (j > highestI - 1)
    return -1;
  return j - i;
}

