#pragma once

#include "Mode.h"

class Chatbox : public Mode
{
public:
    void init() override;
    void loop() override;
};
