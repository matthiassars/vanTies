#include "AdditiveOscillator.h"

using namespace std;
using namespace rack;
using namespace dsp;

void AdditiveOscillator::setFreq(float freq) {
  dPhase[0] = freq * APP->engine->getSampleTime();
  dPhase[2] = stretch * dPhase[0];
  dPhase[1] = dPhase[0] + dPhase[2];
}

// Quantize the stretch parameter to consonant intervals.
float AdditiveOscillator::quantStretch
(float stretch, AdditiveOscillator::StretchQuant stretchQuant) {
  if (stretchQuant == AdditiveOscillator::StretchQuant::CONSONANTS) {
    stretch += 1.f;
    bool stretchNegative = false;
    if (stretch < 0.f) {
      stretch = -stretch;
      stretchNegative = true;
    }
    if (stretch < 1.f / 16.f)
      stretch = 0.f;
    else if (stretch < 3.f / 16.f)
      stretch = 1.f / 8.f; // 3 octaves below
    else if (stretch < 7.f / 24.f)
      stretch = 1.f / 4.f; // 2 octaves below
    else if (stretch < 5.f / 12.f)
      stretch = 1.f / 3.f; // octave + fifth below
    else if (stretch < 7.f / 12.f)
      stretch = 1.f / 2.f; // octave below
    else if (stretch < 17.f / 24.f)
      stretch = 2.f / 3.f; // fifth below
    else {
      int stretchOctave = 0;
      while (stretch > 2.f) {
        stretch *= .5f;
        stretchOctave++;
      }
      while (stretch < 1.f) {
        stretch *= 2.f;
        stretchOctave--;
      }
      if (stretch < 11.f / 10.f)
        stretch = 1.f; // prime
      else if (stretch < 49.f / 40.f)
        stretch = 6.f / 5.f; // minor third
      else if (stretch < 31.f / 24.f)
        stretch = 5.f / 4.f; // major third
      else if (stretch < 17.f / 12.f)
        stretch = 4.f / 3.f; // perfect fourth
      else if (stretch < 31.f / 20.f)
        stretch = 3.f / 2.f; // perfect fifth
      else if (stretch < 49.f / 30.f)
        stretch = 8.f / 5.f; // minor sixth
      else if (stretch < 11.f / 6.f)
        stretch = 5.f / 3.f; // major sixth
      else
        stretch = 2.f; // octave
      stretch *= exp2(stretchOctave);
    }
    if (stretchNegative)
      stretch = -stretch;
    stretch -= 1.f;
  } else if (stretchQuant == AdditiveOscillator::StretchQuant::HARMONICS)
    stretch = round(stretch);
  return stretch;
}

float AdditiveOscillator::getWave(size_t i) {
  if (spec->getStereoMode() == SpectrumStereo::StereoMode::MONO
    && i == 1)
    return Oscillator::getWave(0);
  else
    return Oscillator::getWave(i);
}

void AdditiveOscillator::process() {
  if (!spec)
    return;

  incrementPhases();

  // exclude partials oscillating faster than the Nyquist frequency
  int highest;
  if (stretch == 0.f)
    highest = spec->getHighest();
  else
    highest = min(spec->getHighest(),
      (int)floor((.5f / abs(dPhase[0]) - 1.f) / abs(stretch)) + 1);

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
    sine_iMin2 = sinf(TWOPI * phase[0]);
  if (1 < highest) {
    cosine = cosf(TWOPI * phase[2]);
    sine_iMin1 = sinf(TWOPI * phase[1]);
  }
  wave[0] = spec->getAmp(0) * sine_iMin2 + spec->getAmp(1) * sine_iMin1;
  if (spec->getStereoMode() != SpectrumStereo::StereoMode::MONO)
    wave[1] = spec->getAmpR(0) * sine_iMin2 + spec->getAmpR(1) * sine_iMin1;
  float sine_i;
  for (int i = 2; i < highest; i++) {
    sine_i = 2.f * sine_iMin1 * cosine - sine_iMin2;
    wave[0] += spec->getAmp(i) * sine_i;
    if (spec->getStereoMode() != SpectrumStereo::StereoMode::MONO)
      wave[1] += spec->getAmpR(i) * sine_i;
    sine_iMin2 = sine_iMin1;
    sine_iMin1 = sine_i;
  }
}
