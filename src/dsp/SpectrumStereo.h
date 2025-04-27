#pragma once
#include <iostream>
#include "rack.hpp"
#include "Spectrum.h"

// a class for the spectrum
class SpectrumStereo : public Spectrum {
public:
  enum StereoMode {
    MONO,
    SOFT_PAN,
    HARD_PAN
  };

  // Make sure that partialLeft is of length oscs-1
  void init(int oscs, CvBuffer* buf, bool* partialLeft);

  ~SpectrumStereo();

  void set0();

  void setStereoMode(StereoMode stereoMode) {
    this->stereoMode = stereoMode;
  }

  void setFlip(bool flip) { this->flip = flip; }

  bool isFlipped() { return flip; }

  StereoMode getStereoMode() { return stereoMode; }

  float getAmpR(int i);

  bool ampsAre0() { return zeroAmp; }

  void process();

  void smoothen();

  float getAbsAmp(int i);

  int distUp(int i);

  int distDown(int i);

private:
  StereoMode stereoMode;
  bool flip;
  float* ampsR;
  float* ampsRSmooth;
  // array of length oscs-1 (leave out the fundamental)
  bool* partialLeft;
};
