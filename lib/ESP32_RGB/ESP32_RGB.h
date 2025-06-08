#pragma once

#include <Adafruit_NeoPixel.h>

namespace ESP_32 {
    class _RGB {
      private:
        // it's on pin 48 and has just 1 unit
        Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, 48, NEO_GRB + NEO_KHZ800);
        bool begun = false;

      public:
        void setColor(uint8_t r, uint8_t g, uint8_t b) {
            if (!begun) {
                strip.begin();
                begun = true;
            }
            strip.setPixelColor(0, r, g, b);
            strip.show();
        }
    };

    _RGB RGB;
}
