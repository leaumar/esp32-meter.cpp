#pragma once

#include "Mode.h"

class FakeMeter : public Mode {
  public:
    void init() override;
    void loop() override;
};
