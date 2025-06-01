#pragma once

#include "Mode.h"

class Chirp : public Mode {
  public:
    void init() override;
    void loop() override;
};
