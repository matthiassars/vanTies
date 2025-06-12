#pragma once
#include <cmath>
#include <algorithm>
#include <limits.h>

// a class for the CV buffer
class CvBuffer {
public:
  enum Mode {
    LOW_HIGH,
    HIGH_LOW,
    RANDOM
  };

  void init(int size, int oscs, Mode* mode, int maxClock = INT_MAX);

  ~CvBuffer();

  void setLowestHighest(float lowest, float highest);
  // Set the delay time, relative to the buffer size. 1.f is maximum
  void setDelayRel(float delayRel) {
    this->delayRel = std::min(std::max(delayRel, -1.f), 1.f);
  }
  void setOn(bool on) { this->on = on; }
  void setFrozen(bool frozen) { this->frozen = frozen; }
  void setClocked(bool clocked) { this->clocked = clocked; }
  void setClockTrigger(bool clTrigger) { this->clTrigger = clTrigger; }

  int getLowest() { return lowest; }
  int getHighest() { return highest; }
  int getDelay() { return delay; }
  Mode getMode() { return *mode; }
  bool isOn() { return on; }
  bool isFrozen() { return frozen; }
  bool isClocked() { return clocked; }
  bool clockIsTriggered() { return clTrigger; }
  int getClockTime() { return clTime; }
  int getClockMult() { return clMult; }
  float getValue(int i) { return getValue_buf(posRead(i)); }
  int getSize() { return size; }

  void push(float value);
  void empty();
  void randomize();
  virtual void process();
  void resize(int size);

protected:
  float* buf;
  int posWrite = 0;
  int size = 0;
  // delayRel is a float between 0. and 1.. It is the delay time relative to 
  // the maximum, given by the buffer size. 
  float delayRel = 0;
  // the integer delay is going to be the delay time expressed in samples
  int delay = 0;
  int lowest = 1;
  int highest = 1;
  bool on = false;
  Mode* mode;
  bool frozen = false;
  bool randomized = false;

  ////  random  //////////////////////////////////////////////////////////////

  int oscs = 0;
  float* random;

  ////  clock  ///////////////////////////////////////////////////////////////

  bool clocked = false;
  int clTime = 0;
  int clCounter = 0;
  bool clTrigger = false;
  bool clIsTriggered = false;
  int clMult = 0;
  int maxClock = 0;

  ////////////////////////////////////////////////////////////////////////////

  float getValue_buf(int i) {
    return (i >= 0 && i < size) ?
      buf[(size + posWrite - i - 1) % size] :
      0.f;
  }

  int posRead(int i) {
    return (*mode != RANDOM) ?
      delay * ((delay > 0) ? (i - lowest + 1) : (i - highest)) :
      (int)(abs(delay) * random[i % oscs] * ((clocked) ? (highest - lowest) : 1));
  }

  virtual void processClock();
};