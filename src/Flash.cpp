#include <Arduino.h>
#include "Flash.h"
#include "Freenove_WS2812_Lib_for_ESP32.h"

// usb cable provides power and main serial

// this causes a redefine warning but omitting this breaks the led and strip
#define LED_BUILTIN 2
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

// there's 1 rgb led in the strip and it only has channel 0
Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(1, PIN_NEOPIXEL, 0, TYPE_GRB);

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

    for (int i = 0; i < ARRAY_LENGTH(colorSequence); i++)
    {
        strip.setLedColorData(0, colorSequence[i][0], colorSequence[i][1], colorSequence[i][2]);
        strip.show();
        delay(speed);
    }

    delay(500);
}
