#pragma once
#include <vector>
#include "CvBuffer.h"

using namespace std;

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
  float crRatio;

  CvBuffer* buf = nullptr;

  // only for oscs <= 128
  const array<int, 32>
    prime = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
        31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
        73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
        127, 131 };

  void process_tmp() {
    if (!buf)
      return;

    for (int i = 0; i < lowestI - 1; i++)
      amps_tmp[i] = 0.f;
    for (int i = lowestI - 1; i < highestI; i++) {
      if (tilt == -.5f)
        amps_tmp[i] = 1.f / (float)(i + 1);
      else if (tilt == 1.f)
        amps_tmp[i] = 1.f;
      else
        amps_tmp[i] = powf(i + 1, tilt);
    }
    for (int i = highestI; i < oscs; i++)
      amps_tmp[i] = 0.f;

    // fade factors for the lowest and highest partials,
    // in order to make the "partials" and "lowest" parameters act
    // continuously.
    float fadeLowest = lowestI - lowest + 1.f;
    float fadeHighest = highest - highestI;
    amps_tmp[lowestI - 1] *= fadeLowest;
    if (highest < oscs)
      amps_tmp[highestI - 1] *= fadeHighest;

    // Apply the Sieve of Eratosthenes
    // the sieve knob has 2 zones:
    // on the right we keep the primes
    // and go from low to high primes,
    // on the left we sieve the primes themselves too,
    // and go in reversed order.
    // Again, with a fade factor.
    vector<int> sieveFadePartials;
    int sieveI = (int)sieve;
    float sieveFade = 1.f;
    if (keepPrimes) {
      sieveFade = sieveI + 1.f - sieve;
      // loop over all prime numbers up to the one given by the
      // sieve parameter
      for (int i = 0; i < sieveI; i++) {
        // loop over all proper multiples of that prime
        // and sieve those out
        for (int j = 2; j * prime[i] < highestI + 1; j++)
          amps_tmp[j * prime[i] - 1] = 0.f;
      }
      for (int j = 2; j * prime[sieveI] < highestI + 1; j++) {
        amps_tmp[j * prime[sieveI] - 1] *= sieveFade;
        sieveFadePartials.push_back(j * prime[sieveI] - 1);
      }
    } else {
      sieveFade = sieve - sieveI;
      for (int i = sieveI + 1; i < 32; i++) {
        for (int j = 1; j * prime[i] < highestI + 1; j++)
          amps_tmp[j * prime[i] - 1] = 0.f;
      }
      for (int j = 1; j * prime[sieveI] < highestI + 1; j++) {
        amps_tmp[j * prime[sieveI] - 1] *= sieveFade;
        sieveFadePartials.push_back(j * prime[sieveI] - 1);
      }
    }

    // apply the CV buffer
    // and at the same time compute the sum of the amplitudes,
    // in order to normalize later on
    float sumAmp = 0.f;
    for (int i = lowestI - 1; i < highestI; i++) {
      if (buf->isOn())
        amps_tmp[i] *= buf->getValue(i);
      sumAmp += abs(amps_tmp[i]);
    }
    zeroAmp = (sumAmp < 1.e-4f);
    if (zeroAmp)
      return;

    // normalize the amplitudes
    // apply the fade factors again
    for (int i = lowestI - 1; i < highestI; i++) {
      amps_tmp[i] /= sumAmp;
      if (buf->isOn())
        amps_tmp[i] *= buf->getValue(i);
    }
    amps_tmp[lowestI - 1] *= fadeLowest;
    if (highest < oscs)
      amps_tmp[highestI - 1] *= fadeHighest;
    for (int i : sieveFadePartials)
      amps_tmp[i] *= sieveFade;
  }

public:
  void init(int oscs, CvBuffer* buf) {
    oscs = max(oscs, 0);
    this->oscs = oscs;
    amps_tmp = new float[oscs];
    amps = new float[oscs];
    ampsSmooth = new float[oscs];
    set0();
    this->buf = buf;
  }

  ~Spectrum() {
    delete amps_tmp;
    delete amps;
    delete ampsSmooth;
  }

  void set0() {
    for (int i = 0; i < oscs; i++) {
      amps_tmp[i] = 0.f;
      amps[i] = 0.f;
      ampsSmooth[i] = 0.f;
    }
  }

  void setLowestHighest(float lowest, float highest) {
    this->lowest = clamp(lowest, 1.f, (float)oscs);;
    this->highest = clamp(highest, lowest, (float)oscs);;
    lowestI = clamp((int)lowest, 1, oscs);
    highestI = clamp((int)highest, 1, oscs);
  }
  void setTilt(float tilt) { this->tilt = tilt; }
  void setSieve(float sieve) { this->sieve = sieve; }
  void setKeepPrimes(bool keepPrimes) { this->keepPrimes = keepPrimes; }
  void setCRRatio(float crRatio) { this->crRatio = crRatio; }

  int getLowest() { return lowestI; }
  int getHighest() { return highestI; }

  float getAmp(int i) {
    if (i < 0 || i >= oscs || zeroAmp)
      return 0.f;
    return ampsSmooth[i];
  }

  bool ampsAre0() { return zeroAmp; }

  void process() {
    process_tmp();
    for (int i = 0; i < oscs; i++)
      amps[i] = amps_tmp[i];
  }

  void smoothen() {
    for (int i = 0; i < oscs; i++) {
      ampsSmooth[i] += crRatio * (amps[i] - ampsSmooth[i]);
    }
  }
};
