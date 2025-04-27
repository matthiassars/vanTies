#include "Spectrum.h"

using namespace std;
using namespace rack;
using namespace dsp;

void Spectrum::process_tmp() {
  if (!buf)
    return;

  for (int i = 0; i < lowestI - 1; i++)
    amps_tmp[i] = 0.f;
  for (int i = lowestI - 1; i < highestI; i++) {
    if (abs(tilt + 1.f) < 1.e-3f)
      amps_tmp[i] = 1.f / (float)(i + 1);
    else if (tilt == 0.f)
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
  if (highestI < oscs + 1)
    amps_tmp[highestI - 1] *= fadeHighest;

  // Apply the Sieve of Eratosthenes
  // the sieve knob has 2 zones:
  // on the right we keep the primes
  // and go from low to high primes,
  // on the left we sieve the primes themselves too,
  // and go in reversed order.
  // Again, with a fade factor.
  int sieveI = (int)sieve;
  float sieveFade = 1.f;
  if (keepPrimes) {
    sieveFade = sieveI + 1.f - sieve;
    // loop over all prime numbers up to the one given by the
    // sieve parameter
    for (int i = 0; i < sieveI; i++) {
      // loop over all proper multiples of that prime
      // and sieve those out
      for (int j = 2; j * PRIME[i] < highestI + 1; j++)
        amps_tmp[j * PRIME[i] - 1] = 0.f;
    }
    for (int j = 2; j * PRIME[sieveI] < highestI + 1; j++)
      amps_tmp[j * PRIME[sieveI] - 1] *= sieveFade;
  } else {
    sieveFade = sieve - sieveI;
    for (int i = sieveI + 1; i < 32; i++) {
      for (int j = 1; j * PRIME[i] < highestI + 1; j++)
        amps_tmp[j * PRIME[i] - 1] = 0.f;
    }
    for (int j = 1; j * PRIME[sieveI] < highestI + 1; j++)
      amps_tmp[j * PRIME[sieveI] - 1] *= sieveFade;
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
  zeroAmp = (sumAmp < 1.e-3f);
  if (zeroAmp)
    return;

  // normalize the amplitudes
  // apply the comb filter
  // and apply the fade factors again
  for (int i = lowestI - 1; i < highestI; i++) {
    amps_tmp[i] /= sumAmp;
    if (abs(comb) > 1.e-3f)
      amps_tmp[i] *= .5f * cosf(M_PI * comb * ((i + 1) - lowest)) + .5f;
    if (buf->isOn())
      amps_tmp[i] *= buf->getValue(i);
  }

  amps_tmp[lowestI - 1] *= fadeLowest;
  if (highestI < oscs + 1)
    amps_tmp[highestI - 1] *= fadeHighest;

  if (keepPrimes) {
    for (int j = 2; j * PRIME[sieveI] < highestI + 1; j++)
      amps_tmp[j * PRIME[sieveI] - 1] *= sieveFade;
  } else {
    for (int j = 1; j * PRIME[sieveI] < highestI + 1; j++)
      amps_tmp[j * PRIME[sieveI] - 1] *= sieveFade;
  }
}

void Spectrum::init(int oscs, CvBuffer* buf) {
  oscs = max(oscs, 0);
  this->oscs = oscs;
  amps_tmp = new float[oscs];
  amps = new float[oscs];
  ampsSmooth = new float[oscs];
  set0();
  this->buf = buf;
}

Spectrum::~Spectrum() {
  delete amps_tmp;
  delete amps;
  delete ampsSmooth;
}

void Spectrum::set0() {
  for (int i = 0; i < oscs; i++) {
    amps_tmp[i] = 0.f;
    amps[i] = 0.f;
    ampsSmooth[i] = 0.f;
  }
}

void Spectrum::setLowestHighest(float lowest, float highest) {
  this->lowest = clamp(lowest, 1.f, (float)oscs);
  this->highest = clamp(highest, lowest, (float)(oscs + 1));
  lowestI = clamp((int)lowest, 1, oscs);
  highestI = clamp((int)highest, 1, oscs + 1);
}

float Spectrum::getAmp(int i) {
  if (i < 0 || i >= oscs || zeroAmp)
    return 0.f;
  return ampsSmooth[i];
}

void Spectrum::process() {
  process_tmp();
  for (int i = 0; i < oscs; i++)
    amps[i] = amps_tmp[i];
}

void Spectrum::smoothen() {
  for (int i = 0; i < oscs; i++)
    ampsSmooth[i] += blockRatio * (amps[i] - ampsSmooth[i]);
}
