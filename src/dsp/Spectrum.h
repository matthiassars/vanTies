#pragma once
#include "CvBuffer.h"

// a class for the spectrum
class Spectrum {
public:
  enum StereoMode {
    MONO,
    SOFT_PAN,
    HARD_PAN
  };

  void init(int oscs, CvBuffer* buf, int channels = 0, int* partialChan = nullptr);

  ~Spectrum();

  void set0();

  inline void setLowestHighest(float lowest, float highest) {
    this->lowest = std::min(std::max(lowest, 1.f), (float)oscs);
    this->highest = std::min(std::max(highest, lowest), (float)(oscs + 1));
    lowestI = std::min(std::max((int)lowest, 1), oscs);
    highestI = std::min(std::max((int)highest, 1), oscs);
  }
  inline void setTilt(float tilt) {
    this->tilt = tilt;
  }
  inline void setSieve(float sieve) {
    this->sieve = sieve;
  }
  inline void setKeepPrimes(bool keepPrimes) {
    this->keepPrimes = keepPrimes;
  }
  inline void setComb(float comb) {
    this->comb = comb;
  }
  inline void setSmoothCoeff(float smoothCoeff) {
    this->smoothCoeff = smoothCoeff;
  }
  inline void setStereoMode(StereoMode stereoMode) {
    this->stereoMode = stereoMode;
  }

  inline int getLowest() { return lowestI; }
  inline int getHighest() { return highestI; }
  inline bool ampsAre0() { return zeroAmp; }
  inline float getAmp(int i, int c = 0) { return ampsSmooth[i + c * oscs]; }
  StereoMode getStereoMode() { return stereoMode; }

  void process();
  void smoothen();

protected:
  StereoMode stereoMode = MONO;
  int channels = 0;
  int oscs = 0;
  float lowest = 1.f;
  float highest = 0.f;
  float tilt = 0.f;
  float sieve = 0.f;
  bool keepPrimes = true;
  int lowestI = 1;
  int highestI = 0;
  // an array we do our computations on:
  float* amps_tmp;
  // an array for the result of these computations:
  float* amps;
  // and an array where things are smoothened out (since we won't do these
  // at audio rate):
  float* ampsSmooth;
  bool zeroAmp = true;
  float comb = 0.f;
  float smoothCoeff;
  // number of output channels (1 for mono, 2 for stereo)
  int* partialChan;

  CvBuffer* buf = nullptr;

  // only for oscs <= 128
  const int PRIME[32] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
    31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    127, 131
  };
};
