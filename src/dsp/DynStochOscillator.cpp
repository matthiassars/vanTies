#include "DynStochOscillator.h"

using namespace std;

DynStochOscillator::DynStochOscillator() {
  reset(false);
}

void DynStochOscillator::reset(bool randomize) {
  Oscillator::reset();

  float a0 = (randomize) ? rand(-1.f, 1.f) : 0.f;
  breakpoints = { Breakpoint(0.f, a0), Breakpoint(1.f, a0) };

  while (breakpoints.size() < segments + 1)
    insertBreakpoint(randomize);

  nextBp = breakpoints.begin();
  nextBp++;

  computeSlope();
}

void DynStochOscillator::setSegments(int segments) {
  segments = max(segments, 2);

  while (breakpoints.size() < static_cast<size_t>(segments + 1))
    insertBreakpoint();
  while (breakpoints.size() > static_cast<size_t>(segments + 1))
    eraseBreakpoint();

  this->segments = static_cast<int>(breakpoints.size()) - 1;
  segmentsInv = 1.f / this->segments;

  if (nextBp == breakpoints.end())
    nextBp = next(breakpoints.begin());
  computeSlope();
}

void DynStochOscillator::process() {
  const float len = getLength();
  if (len <= EPS) {
    wave[0] = 0.f;
    return;
  }

  wave[0] = (dPh[0] < .5f * len) ? intercept + slope * ph[0] : 0.f;

  ph[0] += dPh[0];

  // find the correct breakpoint
  while (ph[0] >= nextBp->t && nextBp != prev(breakpoints.end())) {
    nextBp++;
    computeSlope();
    if (prev(nextBp) != breakpoints.begin())
      wiggle(prev(nextBp));
  }

  // move the phasor back into the function domain
  while (ph[0] >= len) {
    ph[0] -= len;
    nextBp = breakpoints.begin();
    wiggleEdgeBp();
  }

  // find the correct breakpoint
  while (ph[0] >= nextBp->t) {
    nextBp++;
    computeSlope();
    if (prev(nextBp) != breakpoints.begin())
      wiggle(prev(nextBp));
  }
}

void DynStochOscillator::insertBreakpoint(bool random) {
  float t = rand(0.f, 1.f);
  float a = (random) ? rand(-1.f, 1.f) : refFunction(t);
  breakpoints.push_back(Breakpoint(t, a));
  breakpoints.sort();
}

void DynStochOscillator::eraseBreakpoint() {
  auto prevBp = prev(nextBp);
  auto target = (prevBp != breakpoints.begin()) ? prevBp : prev(breakpoints.end(), 2);
  if (target == nextBp)
    nextBp = next(target);
  breakpoints.erase(target);
}

void DynStochOscillator::computeSlope() {
  auto prevBp = prev(nextBp);
  const float dt = nextBp->t - prevBp->t;
  if (fabsf(dt) <= EPS) {
    slope = 0.f;
    intercept = prevBp->a;
    return;
  }

  slope = (nextBp->a - prevBp->a) / dt;
  intercept = prevBp->a - prevBp->t * slope;
}

void DynStochOscillator::wiggle(std::list<Breakpoint>::iterator i) {
  // wiggle time component
  float d = 1.f - getLength();
  i->t = mirror(
    rand(i->t - randAmt_t, i->t + randAmt_t),
    // rand(i->t - randAmt_t + d * randAmt_t * (1.f - destruct_t) * segmentsInv,
    //   i->t + randAmt_t + d * randAmt_t * (1.f - destruct_t) * segmentsInv),
    prev(i)->t, next(i)->t
  );

  // wiggle amp component
  d = refFunction(i->t) - i->a;
  i->a = mirror((d < 0.f) ?
    rand(
      i->a + (d + (-1.f - d) * destruct_a) * randAmt_a,
      i->a + destruct_a * randAmt_a) :
    rand(
      i->a - destruct_a * randAmt_a,
      i->a + (d + (1.f - d) * destruct_a) * randAmt_a),
    -1.f, 1.f
  );
}

void DynStochOscillator::wiggleEdgeBp() {
  // wiggle time component
  float d = 1.f - getLength();
  prev(breakpoints.end())->t = mirror((d < 0.f) ?
    rand(
      getLength() + (d + (-1.f - d) * destruct_t) * randAmt_t,
      getLength() + destruct_t * randAmt_t) :
    rand(
      getLength() - destruct_t * randAmt_t,
      getLength() + (d + (1.f - d) * destruct_t) * randAmt_t),
    // no frequencies higher than Nyquist
    max(prev(breakpoints.end(), 2)->t, 4.f * (float)dPh[0]),
    // no frequencies lower that 2 octaves below
    min(next(breakpoints.begin())->t + getLength(), 2.f)
  );
  auto last = prev(breakpoints.end());
  const float minLastT = max(prev(breakpoints.end(), 2)->t + EPS, 4.f * (float)dPh[0]);
  last->t = max(last->t, minLastT);

  // wiggle amp component
  d = -breakpoints.front().a;
  breakpoints.begin()->a = mirror((d < 0.f) ?
    rand(
      breakpoints.front().a + (d + (-1.f - d) * destruct_a) * randAmt_a,
      breakpoints.front().a + destruct_a * randAmt_a) :
    rand(
      breakpoints.front().a - destruct_a * randAmt_a,
      breakpoints.front().a + (d + (1.f - d) * destruct_a) * randAmt_a),
    -1.f, 1.f
  );
  prev(breakpoints.end())->a = breakpoints.front().a;
}

float DynStochOscillator::refFunction(float x) {
  if (refWaveform == SINE)
    return sinf(TWOPI * x / getLength());
  return 0.f;
}