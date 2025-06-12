#include "Spectrum.h"

using namespace std;

void Spectrum::init(int oscs, CvBuffer* buf, int channels, int* partialChan) {
  oscs = max(oscs, 0);
  this->oscs = oscs;
  channels = max(channels, 0);
  this->channels = channels;
  amps_tmp = new float[oscs + 1];
  amps = new float[channels * oscs];
  ampsSmooth = new float[channels * oscs];
  set0();
  this->partialChan = partialChan;
  if (!partialChan)
    stereoMode = MONO;
  this->buf = buf;
}

Spectrum::~Spectrum() {
  delete amps_tmp;
  delete amps;
  delete ampsSmooth;
}

void Spectrum::set0() {
  for (int i = 0; i < oscs + 1; i++)
    amps_tmp[i] = 0.f;
  int arraySize = channels * oscs;
  for (int i = 0; i < arraySize; i++) {
    amps[i] = 0.f;
    ampsSmooth[i] = 0.f;
  }
}

void Spectrum::smoothen() {
  for (int i = 0; i < channels * oscs; i++)
    ampsSmooth[i] += smoothCoeff * (amps[i] - ampsSmooth[i]);
}

void Spectrum::process() {
  for (int i = 0; i < lowestI - 1; i++)
    amps_tmp[i] = 0.f;
  for (int i = lowestI - 1; i < highestI; i++)
    amps_tmp[i] = powf(i + 1, tilt);
  for (int i = highestI; i < oscs + 1; i++)
    amps_tmp[i] = 0.f;

  // fade factors for the lowest and highest partials,
  // in order to make the "partials" and "lowest" parameters act
  // continuously.
  float fadeLowest = lowestI - lowest + 1.f;
  float fadeHighest = highest - highestI;
  amps_tmp[lowestI - 1] *= fadeLowest;
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
  // in order to normalize
  float sumAmp = 0.f;
  for (int i = lowestI - 1; i < highestI; i++) {
    if (buf->isOn())
      amps_tmp[i] *= buf->getValue(i);
    sumAmp += abs(amps_tmp[i]);
  }

  zeroAmp = (sumAmp < 1.e-6f);
  if (zeroAmp) {
    for (int i = lowestI - 1; i < highestI; i++)
      amps_tmp[i] = 0.f;
  } else {
    // normalize the amplitudes
    // apply the comb filter
    // and apply the fade factors again
    for (int i = lowestI - 1; i < highestI; i++) {
      amps_tmp[i] /= sumAmp;
      amps_tmp[i] *= .5f * cosf(M_PI * comb * ((i + 1) - lowest)) + .5f;
      if (buf->isOn())
        amps_tmp[i] *= buf->getValue(i);
    }

    amps_tmp[lowestI - 1] *= fadeLowest;
    amps_tmp[highestI - 1] *= fadeHighest;

    for (int j = (keepPrimes) ? 2 : 1; j * PRIME[sieveI] < highestI + 1; j++)
      amps_tmp[j * PRIME[sieveI] - 1] *= sieveFade;
  }

  // copy the amplitude values to amps[] and apply panning
  if (stereoMode == MONO) { // mono mode
    for (int c = 0; c < channels; c++) {
      for (int i = 0; i < oscs; i++)
        amps[i + oscs * c] = amps_tmp[i];
    }
  } else if (stereoMode == SOFT_PAN) { // soft panned mode
    for (int c = 0; c < channels; c++) {
      // fundamental is present in all channels
      amps[oscs * c] = amps_tmp[0];
      float l = (lowest > 2.f) ? lowest - 2.f : 0.f;
      for (int i = 1; i < oscs; i++)
        amps[i + oscs * c] =
        (c == partialChan[i - 1]) ?
        amps_tmp[i] :
        ((i + 1 > l) ? amps_tmp[i] / sqrtf(i + 1.f - l) : 0.f);
    }
  } else { // hard panned mode
    for (int c = 0; c < channels; c++) {
      // fundamental is present in all channels
      amps[oscs * c] = amps_tmp[0];
      for (int i = 1; i < oscs; i++) {
        amps[i + oscs * c] =
          (c == partialChan[i - 1]) ?
          amps_tmp[i] :
          0.f;
      }
    }
  }
}