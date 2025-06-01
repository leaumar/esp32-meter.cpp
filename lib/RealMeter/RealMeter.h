#pragma once

#include "Mode.h"

class RealMeter : public Mode {
  public:
    void init() override;
    void loop() override;
};
