#include <Arduino.h>
#include "Flash.h"
#include <Adafruit_NeoPixel.h>
#include <iterator>

// usb cable provides power and main serial

// SDK sets pin to 97 but it's actually 2
#undef LED_BUILTIN
#define LED_BUILTIN 2

#define PIN_NEOPIXEL 48

// there's 1 rgb led in the strip and it only has channel 0
Adafruit_NeoPixel strip(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

int colorSequence[5][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255}, {0, 0, 0}};

void Flash::init()
{
    pinMode(LED_BUILTIN, OUTPUT);

    strip.begin();
    strip.setBrightness(10);

    Serial.begin(115200);
    Serial.println("ESP32S3 initialization completed!");
}

void Flash::loop()
{
    int speed = 200;

    Serial.printf("Running time : %.1f s\r\n", millis() / 1000.0f);
    delay(speed);

    digitalWrite(LED_BUILTIN, HIGH);
    delay(speed);
    digitalWrite(LED_BUILTIN, LOW);
    delay(speed);

    for (int i = 0; i < std::size(colorSequence); i++)
    {
        strip.setPixelColor(0, colorSequence[i][0], colorSequence[i][1], colorSequence[i][2]);
        strip.show();
        delay(speed);
    }

    delay(500);
}
