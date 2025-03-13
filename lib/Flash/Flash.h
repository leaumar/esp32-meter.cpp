#pragma once

#include "Mode.h"

class Flash : public Mode
{
public:
    void init() override;
    void loop() override;
};
