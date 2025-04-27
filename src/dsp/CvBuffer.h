#pragma once
#include <iostream>
#include "rack.hpp"

// a class for the CV buffer
class CvBuffer {
public:
  enum Mode {
    LOW_HIGH,
    HIGH_LOW,
    RANDOM
  };

  void init(int size, int oscs, Mode* mode);

  ~CvBuffer();

  void setLowestHighest(float lowest, float highest);
  // Set the delay time, relative to the buffer size. 1.f is maximum
  void setDelayRel(float delayRel);
  void setOn(bool on) { this->on = on; }
  void setFrozen(bool frozen) { this->frozen = frozen; }
  void unfreeze() { frozen = true; }
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
  bool clTrigger;
  bool clIsTriggered = false;
  int clMult = 0;

  ////////////////////////////////////////////////////////////////////////////

  float getValue_buf(int i);
  int posRead(int i);
  virtual void processClock();
};
