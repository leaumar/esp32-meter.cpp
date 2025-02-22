#include <Arduino.h>

#include "Mode.h"
#include "Chatbox.h"
#include "Flash.h"
#include "FakeMeter.h"
#include "RealMeter.h"

Mode *mode = new RealMeter();

void setup()
{
    mode->init();
}

void loop()
{
    mode->loop();
}
