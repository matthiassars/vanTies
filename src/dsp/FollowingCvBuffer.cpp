#include "FollowingCvBuffer.h"

using namespace std;

void FollowingCvBuffer::process() {
  if (masterCvBuffer) {
    setLowestHighest(masterCvBuffer->getLowest(),
      masterCvBuffer->getHighest());
    if (followMode == FollowMode::GET_DELAY_TIME)
      delay = masterCvBuffer->getDelay();
    else
      CvBuffer::process();
  } else {
    setLowestHighest(1, 16);
    CvBuffer::process();
  }
}

void FollowingCvBuffer::processClock() {
  if (masterCvBuffer
    && followMode == FollowMode::SYNC
    && masterCvBuffer->isClocked())
    clTime = masterCvBuffer->getClockTime();
  else
    CvBuffer::processClock();
}
