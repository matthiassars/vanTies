#pragma once
#include "Oscillator.h"
#include "SpectrumStereo.h"

// a class for the additive oscillator

class AdditiveOscillator : public Oscillator {
private:
  float stretch;
  // We need 2 more phasors. In the "process" method we'll see why.
  double stretchPhase = 0.;
  double phase2 = 0.;
  double dStretchPhase = 0.;
  double dPhase2 = 0.;
  // a second wave for stereo
  float waveR = 0.f;
  
  SpectrumStereo* spec = nullptr;

  void incrementPhase() {
    phase += dPhase;
    phase2 += dPhase2;
    stretchPhase += dStretchPhase;
    phase -= floorf(phase);
    phase2 -= floorf(phase2);
    stretchPhase -= floorf(stretchPhase);
  }

public:
  void init(SpectrumStereo* spec) { this->spec = spec; }

  void setFreq(float freq) {
    dPhase = freq * APP->engine->getSampleTime();
    dStretchPhase = stretch * dPhase;
    dPhase2 = dPhase + dStretchPhase;
  }
  void setStretch(float stretch) { this->stretch = stretch; }

  float getStretch() { return stretch; }

  float getWave() {
    if (!spec)
      return 0.f;
    return wave;
  }

  float getWaveR() {
    if (!spec)
      return 0.f;
    if (spec->getStereoMode() == 0)
      return wave;
    else
      return waveR;
  }

  void reset() {
    phase = 0.;
    phase2 = 0.;
    stretchPhase = 0.;
    wave = 0.f;
    waveR = 0.f;
  }

  void process() override {
    if (!spec)
      return;

    if (spec->ampsAre0()) {
      reset();
      return;
    }

    incrementPhase();

    // exclude partials oscillating faster than the Nyquist frequency
    int highest = min(spec->getHighest(),
      (int)floor((.5f / abs(dPhase) - 1.f) / abs(stretch)) + 1);

    // We compute the waves in a smarter way than computing a bunch of
    // sines bute force.
    // We use the identity sin(a+b) = 2*sin(a)*cos(b) - sin(a-b) :
    // sin((1+i*stretch)*phase)
    //   = 2*sin((1+(i-1)*s)*phase)*cos(stretch*phase)
    //     - sin((1+(i-2)*stretch)*phase) .
    // So we can compute the sines iteratively.
    // We define: sine_i := sin((1+i*stretch)*phase) ,
    //   sine_iMin1 := sin((1+(i-1)*stretch)*phase) ,
    //   sine_iMin2 := sin((1+(i-2)*stretch)*phase) ,
    //   cosine := cos(stretch*phase) = sin(stretch*phase+pi/2),
    // so using our identity, we can covmpute:
    // sine_i := 2*sine_iMin1*cosine - sine_iMin2 .
    // We have 3 independent trigonometric functions, so we have 3
    // intependant phasors paramaters: phase,
    // phase2 := (1+stretch)*phase and stretchPase := stretch*phase .
    float cosine = 0.f;
    float sine_iMin2 = 0.f;
    float sine_iMin1 = 0.f;
    if (0 < highest)
      sine_iMin2 = sin2piBhaskara(phase);
    if (1 < highest) {
      cosine = sin2piBhaskara(stretchPhase + .25f);
      sine_iMin1 = sin2piBhaskara(phase2);
    }
    wave = spec->getAmp(0) * sine_iMin2 + spec->getAmp(1) * sine_iMin1;
    if (spec->getStereoMode() != 0)
      waveR = spec->getAmpR(0) * sine_iMin2 + spec->getAmpR(1) * sine_iMin1;
    float sine_i;
    for (int i = 2; i < highest; i++) {
      sine_i = 2.f * sine_iMin1 * cosine - sine_iMin2;
      wave += spec->getAmp(i) * sine_i;
      if (spec->getStereoMode() != 0)
        waveR += spec->getAmpR(i) * sine_i;
      sine_iMin2 = sine_iMin1;
      sine_iMin1 = sine_i;
    }
  }
};
