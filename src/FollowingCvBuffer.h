#pragma once
#include "CvBuffer.h"

using namespace std;
using namespace dsp;

class FollowingCvBuffer : public CvBuffer {
private:
  CvBuffer* masterCvBuffer = nullptr;

public:
  // 0: free; 1: sync clock; 3: get delay time
  int followMode = 0;

  void setMasterCvBuffer(CvBuffer* masterCvBuffer) {
    this->masterCvBuffer = masterCvBuffer;
  }

  void process() override {
    if (masterCvBuffer) {
      setLowestHighest(masterCvBuffer->getLowest(),
        masterCvBuffer->getHighest());
      if (masterCvBuffer && followMode == 2)
        delay = masterCvBuffer->getDelay();
    }
    CvBuffer::process();
  }

  void processClock() override {
    if (masterCvBuffer && followMode == 1 && masterCvBuffer->isClocked()) {
      clTime = masterCvBuffer->getClockTime();
      clTimeDiv = clTime * clDiv;
    } else
      CvBuffer::processClock();
  }
};
