#pragma once
#include "CvBuffer.h"

class FollowingCvBuffer : public CvBuffer {
public:
  enum FollowMode {
    FREE,
    SYNC,
    GET_DELAY_TIME
  };

  FollowMode followMode = FREE;

  void setMasterCvBuffer(CvBuffer* masterCvBuffer) {
    this->masterCvBuffer = masterCvBuffer;
  }

  void setFrozen(bool frozen) {
    this->frozen =
      (masterCvBuffer && followMode == GET_DELAY_TIME) ?
      masterCvBuffer->isFrozen() :
      frozen;
  }

  void process() override;

  void processClock() override;

private:
  CvBuffer* masterCvBuffer = nullptr;
};
