#include "Chatbox.h"
#include "Chirp.h"
#include "FakeMeter.h"
#include "Flash.h"
#include "Mode.h"
#include "RealMeter.h"

Mode *mode = new RealMeter();

void setup() {
    mode->init();
}

void loop() {
    mode->loop();
}
