#pragma once

#include <pins_arduino.h>

// SDK sets pin to 97 but it's actually 2
#undef LED_BUILTIN
#define LED_BUILTIN 2
