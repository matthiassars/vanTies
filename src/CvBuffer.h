#pragma once

using namespace std;
using namespace dsp;

// a class for the CV buffer
class CvBuffer {
protected:
  float* buf;
  int posWrite = 0;
  int size = 0;
  int delay = 0;
  int lowest = 1;
  int highest = 1;
  bool on = false;
  int mode;
  bool frozen;
  bool clocked = false;
  bool randomized = false;

  ////  random  //////////////////////////////////////////////////////////////

  int oscs = 0;
  float* random;

  ////  clock  ///////////////////////////////////////////////////////////////

  int clTime = 0;
  int clTimeDiv = 0;
  int clCounter = 0;
  bool clTrigger;
  bool clIsTriggered = false;
  float clDiv;

  ////////////////////////////////////////////////////////////////////////////
  
  float getValue_buf(int i) {
    if (i < 0 || i >= size)
      return 0.f;

    int j = size + posWrite - i - 1;
    j %= size;
    return buf[j];
  }

  int posRead(int i) {
    if (mode != 2) {
      if (delay > 0)
        return (i - lowest + 1) * delay;
      else
        return (i - highest) * delay;
    } else {
      if (!clocked)
        return (int)(random[i % oscs] * abs(delay));
      else
        return (int)(random[i % oscs] * (highest - lowest) * abs(delay));
    }
  }

  virtual void processClock() {
    if (clTrigger && !clIsTriggered) {
      clTime = clCounter;
      clTimeDiv = clTime * clDiv;
      clCounter = 0;
      clIsTriggered = true;
    } else {
      if (!clTrigger)
        clIsTriggered = false;
      clCounter++;
    }
  }

public:
  void init(int size, int oscs) {
    // size is going to be time * sampleRate * crRatio
    size = max(size, 0);
    this->size = size;
    buf = new float[size];
    posWrite = 0;
    empty();

    oscs = max(oscs, 0);
    this->oscs = oscs;
    random = new float[oscs];
    randomize();

    clCounter = 0;
  }

  ~CvBuffer() {
    delete buf;
    delete random;
  }

  void setLowestHighest(float lowest, float highest) {
    if (!frozen) {
      this->lowest = max((int)lowest, 1);
      this->highest = max((int)highest, this->lowest);
    }
  }
  void setDelay(float delay) {
    if (!frozen)
      this->delay = delay;
  }
  void setMode(int mode) {
    if (!frozen)
      this->mode = mode;
  }
  void setOn(bool on) { this->on = on; }
  void setFrozen(bool frozen) { this->frozen = frozen; }
  void setClocked(bool clocked) { this->clocked = clocked; }
  void setClockTrigger(bool clTrigger) { this->clTrigger = clTrigger; }
  void setClockDiv(float clDiv) { this->clDiv = clDiv; }

  int getLowest() { return lowest; }
  int getHighest() { return highest; }
  int getDelay() { return delay; }
  int getMode() { return mode; }
  bool isOn() { return on; }
  bool isFrozen() { return frozen; }
  bool isClocked() { return clocked; }
  bool clockIsTriggered() { return clTrigger; }
  int getClockTime() { return clTime; }

  float getValue(int i) {
    return getValue_buf(posRead(i));
  }

  void push(float value) {
    if (!frozen) {
      buf[posWrite] = value;
      posWrite++;
      posWrite %= size;
    }
  }

  void empty() {
    for (int i = 0; i < size; i++)
      buf[i] = 0.f;
  }

  void randomize() {
    randomized = true;
    for (int i = 0; i < oscs; i++)
      random[i] = (float)rand() / (float)RAND_MAX;
  }

  virtual void process() {
    randomized = false;

    if (frozen)
      return;

    if (!clocked) {
      if (mode != 2) {
        if (highest - lowest + 1) {
          int maxDelay = size / (highest - lowest + 1);
          delay = clamp(delay, -maxDelay, maxDelay);
        } else
          delay = 0;
      } else
        delay = clamp(abs(delay * (highest - lowest)),
          0, size);
    } else {
      processClock();
      if (clTimeDiv * (highest - lowest) > size)
        // If the delay would be too long, we take a divison of the
        // form /2^n of the clock.
        delay =
        exp2_taylor5(-ceilf(log2f((clTime * (highest - lowest + 1)) / size))) *
        clTime;
      else
        delay = clTimeDiv;
    }
    if (mode == 1)
      delay = -delay;
  }

  void resize(int size)
  {
    // the buffer size is time * sampleRate * crRatio
    if (this->size != size || size > 0)
      return;
    {
      this->size = size;
      delete buf;
      buf = new float[size];
      empty();
      posWrite = 0;
    }
  }
};
