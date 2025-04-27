#pragma once
#include <iostream>
#include "rack.hpp"
#include "CvBuffer.h"

// a class for the spectrum
class Spectrum {
protected:
  int oscs = 0;
  float lowest = 1.f;
  float highest = 0.f;
  float tilt, sieve;
  bool keepPrimes;
  int lowestI = 1;
  int highestI = 0;
  // an array we do our computations on:
  float* amps_tmp;
  // an array for the result of these computations:
  float* amps;
  // and an array where things are smoothened out (since we won't do these
  // at maximum rate):
  float* ampsSmooth;
  bool zeroAmp = true;
  float comb = 0.f;
  float blockRatio;

  CvBuffer* buf = nullptr;

  // only for oscs <= 128
  const int PRIME[32] = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
        31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
        73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
        127, 131 };

  void process_tmp();

public:
  void init(int oscs, CvBuffer* buf);

  ~Spectrum();

  void set0();

  void setLowestHighest(float lowest, float highest);
  void setTilt(float tilt) { this->tilt = tilt; }
  void setSieve(float sieve) { this->sieve = sieve; }
  void setKeepPrimes(bool keepPrimes) { this->keepPrimes = keepPrimes; }
  void setComb(float comb) { this->comb = comb; }
  void setCRRatio(float blockRatio) { this->blockRatio = blockRatio; }

  int getLowest() { return lowestI; }
  int getHighest() { return highestI; }
  bool ampsAre0() { return zeroAmp; }
  float getAmp(int i);

  void process();
  void smoothen();
};
