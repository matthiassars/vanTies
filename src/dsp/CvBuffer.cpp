#include "CvBuffer.h"

using namespace std;
using namespace rack;
using namespace dsp;

float CvBuffer::getValue_buf(int i) {
  if (i < 0 || i >= size)
    return 0.f;

  int j = size + posWrite - i - 1;
  j %= size;
  return buf[j];
}

int CvBuffer::posRead(int i) {
  if (*mode != RANDOM) {
    if (delay > 0)
      return (i - lowest + 1) * delay;
    else
      return (i - highest) * delay;
  } else {
    if (!clocked)
      return (int)(random[i % oscs] * abs(delay));
    else
      return (int)(random[i % oscs] * (highest - lowest)) * abs(delay);
  }
}

void CvBuffer::processClock() {
  if (clTrigger && !clIsTriggered) {
    clTime = clCounter;
    clCounter = 0;
    clIsTriggered = true;
  } else {
    if (!clTrigger)
      clIsTriggered = false;
    clCounter++;
  }
}

void CvBuffer::init(int size, int oscs, Mode* mode) {
  // size is going to be time * sampleRate * blockRatio
  size = max(size, 0);
  this->size = size;
  buf = new float[size];
  posWrite = 0;
  empty();

  oscs = max(oscs, 0);
  this->oscs = oscs;
  random = new float[oscs];
  randomize();

  this->mode = mode;

  clCounter = 0;
}

CvBuffer::~CvBuffer() {
  delete buf;
  delete random;
}

void CvBuffer::setLowestHighest(float lowest, float highest) {
  this->lowest = max((int)lowest, 1);
  this->highest = max((int)highest, this->lowest);
}

void CvBuffer::setDelayRel(float delayRel) {
  if (!frozen)
    this->delayRel = clamp(delayRel, -1.f, 1.f);
}

void CvBuffer::push(float value) {
  if (!frozen) {
    buf[posWrite] = value;
    posWrite++;
    posWrite %= size;
  }
}

void CvBuffer::empty() {
  for (int i = 0; i < size; i++)
    buf[i] = 0.f;
}

void CvBuffer::randomize() {
  randomized = true;
  for (int i = 0; i < oscs; i++)
    random[i] = (float)rand() / (float)RAND_MAX;
}

void CvBuffer::process() {
  randomized = false;

  if (frozen)
    return;

  if (!clocked) {
    if (*mode != RANDOM)
      delay = delayRel * size / (float)(highest - lowest + 1);
    else
      delay = delayRel * size;
  } else {
    processClock();
    delay = delayRel * size / (float)(highest - lowest + 1);
    if (clTime == 0 || delay == 0)
      delay = 0;
    else {
      if (abs(delay) < clTime) {
        clMult = -abs(round((float)clTime / (float)delay));
        delay = -clTime / clMult;
      } else {
        clMult = abs(round((float)delay / (float)clTime));
        delay = clTime * clMult;
      }
    }
  }

  if (*mode == HIGH_LOW)
    delay = -delay;
}

void CvBuffer::resize(int size) {
  // the buffer size is time * sampleRate * blockRatio
  if (this->size != size || size > 0)
    return;

  this->size = size;
  delete buf;
  buf = new float[size];
  empty();
  posWrite = 0;
}
