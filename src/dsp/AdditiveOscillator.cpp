#include "AdditiveOscillator.h"

using namespace std;

// Quantize the stretch parameter to consonant intervals.
float AdditiveOscillator::quantStretch
(float stretch, StretchQuant stretchQuant) {
  if (stretchQuant == CONSONANTS) {
    stretch += 1.f;
    bool stretchNegative = (stretch < 0.f);
    if (stretchNegative)
      stretch = -stretch;

    if (stretch < 2.f / 3.f)
      stretch =
      (stretch < 1.f / 16.f) ? 0.f :
      (stretch < 3.f / 16.f) ? 1.f / 8.f : // 3 octaves below
      (stretch < 7.f / 24.f) ? 1.f / 4.f : // 2 octaves below
      (stretch < 5.f / 12.f) ? 1.f / 3.f : // octave + fifth below
      (stretch < 7.f / 12.f) ? 1.f / 2.f : // octave below
      2.f / 3.f; // fifth below
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
      stretch =
        (stretch < 11.f / 10.f) ? 1.f : // prime
        (stretch < 49.f / 40.f) ? 6.f / 5.f : // minor third
        (stretch < 31.f / 24.f) ? 5.f / 4.f : // major third
        (stretch < 17.f / 12.f) ? 4.f / 3.f : // perfect fourth
        (stretch < 31.f / 20.f) ? 3.f / 2.f : // perfect fifth
        (stretch < 49.f / 30.f) ? 8.f / 5.f : // minor sixth
        (stretch < 11.f / 6.f) ? 5.f / 3.f : // major sixth
        2.f; // octave
      stretch *= exp2(stretchOctave);
    }

    if (stretchNegative)
      stretch = -stretch;
    stretch -= 1.f;
  } else if (stretchQuant == HARMONICS)
    stretch = round(stretch);

  return stretch;
}

void AdditiveOscillator::process() {
  // exclude partials oscillating faster than the Nyquist frequency
  int highest = (abs(stretch) > 1.e-6f) ?
    min(spec->getHighest(),
      (int)floorf((.5f / abs(dPh[0]) - 1.f) / abs(stretch)) + 1) :
    ((dPh[0] < .5) ? spec->getHighest() : 0);

  // We compute the waves in a smarter way than computing a bunch of
  // sines bute force.
  // We use the identity sin(a+b) = 2*sin(a)*cos(b) - sin(a-b) :
  // sin((1+i*stretch)*ph)
  //   = 2*sin((1+(i-1)*s)*ph)*cos(stretch*ph)
  //     - sin((1+(i-2)*stretch)*ph) .
  // So we can compute the sines iteratively.
  // We define: sine_i := sin((1+i*stretch)*ph) ,
  //   sine_iMin1 := sin((1+(i-1)*stretch)*ph) ,
  //   sine_iMin2 := sin((1+(i-2)*stretch)*ph) ,
  //   cosine := cos(stretch*ph) = sin(stretch*ph+pi/2),
  // so using our identity, we can compute:
  // sine_i := 2*sine_iMin1*cosine - sine_iMin2 .
  // We have 3 independent trigonometric functions, so we have 3
  // intependant phasors: ph[0] := ph,
  // ph[1] := (1+stretch)*ph and ph[2] := stretch*ph .
  float cosine = cosf(TWOPI * ph[2]);
  float sine_iMin2 = (highest > 0) ? sinf(TWOPI * ph[0]) : 0.f;
  float sine_iMin1 = (highest > 1) ? sinf(TWOPI * ph[1]) : 0.f;
  wave[0] = spec->getAmp(0, 0) * sine_iMin2 + spec->getAmp(1, 0) * sine_iMin1;
  wave[1] = spec->getAmp(0, 1) * sine_iMin2 + spec->getAmp(1, 1) * sine_iMin1;
  for (int i = 2; i < highest; i++) {
    float sine_i = 2.f * sine_iMin1 * cosine - sine_iMin2;
    wave[0] += spec->getAmp(i, 0) * sine_i;
    wave[1] += spec->getAmp(i, 1) * sine_i;
    sine_iMin2 = sine_iMin1;
    sine_iMin1 = sine_i;
  }

  incrementPhases();
}

