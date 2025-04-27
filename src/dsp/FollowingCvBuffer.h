#pragma once
#include <iostream>
#include "rack.hpp"
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

  void process() override;

  void processClock() override;

private:
  CvBuffer* masterCvBuffer = nullptr;
};
