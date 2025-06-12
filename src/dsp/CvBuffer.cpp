#include "CvBuffer.h"

using namespace std;

void CvBuffer::processClock() {
  if (clTrigger && !clIsTriggered) {
    clocked = true;
    clTime = clCounter;
    clCounter = 0;
    clIsTriggered = true;
  } else {
    if (!clTrigger)
      clIsTriggered = false;
    clCounter++;
  }

  if (clCounter > maxClock) {
    clocked = false;
    clCounter = 0;
  }
}

void CvBuffer::init(int size, int oscs, Mode* mode, int maxClock) {
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
  this->maxClock = maxClock;
}

CvBuffer::~CvBuffer() {
  delete buf;
  delete random;
}

void CvBuffer::setLowestHighest(float lowest, float highest) {
  this->lowest = max((int)lowest, 1);
  this->highest = max((int)highest, this->lowest);
}

void CvBuffer::push(float value) {
  if (frozen)
    return;

  buf[posWrite] = value;
  posWrite++;
  posWrite %= size;
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

  processClock();

  if (!clocked) {
    if (*mode != RANDOM)
      delay = delayRel * size / (float)(highest - lowest + 1);
    else
      delay = delayRel * size;
  } else {
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

// the buffer size is time * sampleRate * blockRatio
void CvBuffer::resize(int size) {
  if (this->size == size || size < 0)
    return;

  this->size = size;
  delete buf;
  buf = new float[size];
  empty();
  posWrite = 0;
}
