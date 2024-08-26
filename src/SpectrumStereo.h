#pragma once
#include "Spectrum.h"

using namespace std;

// a class for the spectrum
class SpectrumStereo : public Spectrum {
private:
  int stereoMode;
  bool flip;
  float* ampsR;
  float* ampsRSmooth;

  // only for oscs <= 128
  const bool partialLeft[127] = {
      false, true, true, false, false, true, false, true,
      true, false, false, true, false, false, true, false, true, true,
      true, true, true, false, false, false, false, true, false, true,
      true, false, false, false, true, true, false, true, false, true,
      false, false, false, true, true, true, true, false, true, true,
      false, false, false, true, false, false, false, true, false, false,
      true, true, true, true, true, true, true, false, true, false,
      false, true, true, false, false, false, false, false, false, true,
      true, false, true, false, true, false, false, true, true, true,
      false, true, true, false, true, true, true, false, false, false,
      true, true, true, false, false, true, false, true, false, false,
      true, true, false, true, false, false, false, true, true, false,
      false, false, false, false, true, true, true, false, false };

public:
  void init(int oscs, CvBuffer* buf) {
    oscs = max(oscs, 0);
    this->oscs = oscs;
    amps_tmp = new float[oscs];
    amps = new float[oscs];
    ampsR = new float[oscs];
    ampsSmooth = new float[oscs];
    ampsRSmooth = new float[oscs];
    set0();
    this->buf = buf;
  }

  ~SpectrumStereo() {
    delete ampsR;
    delete ampsRSmooth;
  }

  void set0() {
    for (int i = 0; i < oscs; i++) {
      amps_tmp[i] = 0.f;
      amps[i] = 0.f;
      ampsR[i] = 0.f;
      ampsSmooth[i] = 0.f;
      ampsRSmooth[i] = 0.f;
    }
  }

  void setStereoMode(int stereoMode) { this->stereoMode = stereoMode; }
  void setFlip(bool flip) { this->flip = flip; }

  bool isFlipped() { return flip; }
  int getStereoMode() { return stereoMode; }

  float getAmpR(int i) {
    if (i < 0 || i >= oscs || zeroAmp || stereoMode == 0)
      return 0.f;
    return ampsRSmooth[i];
  }

  bool ampsAre0() { return zeroAmp; }

  void process() {
    process_tmp();

    if (stereoMode == 0) { // mono mode
      for (int i = 0; i < oscs; i++)
        amps[i] = amps_tmp[i];
    } else if (stereoMode == 1) { // soft panned mode
      float l = 0.f;
      if (lowest > 2.f)
        l = lowest - 2.f;
      for (int i = 0; i < oscs; i++) {
        if (partialLeft[i - 1] ^ flip) {
          amps[i] = amps_tmp[i];
          if (i + 1 > l)
            ampsR[i] = amps_tmp[i] * 3.f / (i + 3.f - l);
          else
            ampsR[i] = 0.f;
        } else {
          ampsR[i] = amps_tmp[i];
          if (i + 1 > l)
            amps[i] = amps_tmp[i] * 3.f / (i + 3.f - l);
          else
            amps[i] = 0.f;
        }
      }
    } else {
      amps[0] = amps_tmp[0];
      ampsR[0] = amps_tmp[0];
      for (int i = 1; i < oscs; i++) {
        if (partialLeft[i - 1] ^ flip) {
          amps[i] = amps_tmp[i];
          ampsR[i] = 0.f;
        } else {
          amps[i] = 0.f;
          ampsR[i] = amps_tmp[i];
        }
      }
    }
  }

  void smoothen() {
    for (int i = 0; i < oscs; i++) {
      ampsSmooth[i] += crRatio * (amps[i] - ampsSmooth[i]);
      if (stereoMode != 0)
        ampsRSmooth[i] += crRatio * (ampsR[i] - ampsRSmooth[i]);
    }
  }
};
