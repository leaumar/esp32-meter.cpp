#include "Flash.h"

#include "polyfill.h"

#include <ESP32_LED.h>
#include <ESP32_RGB.h>

// usb cable provides power and main serial

int colorSequence[5][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255}, {0, 0, 0}};

void Flash::init() {
    pinMode(LED_BUILTIN, OUTPUT);

    rgbStrip.setBrightness(10);

    Serial.begin(115200);
    Serial.println("ESP32S3 initialization completed!");
}

void Flash::loop() {
    int speed = 200;

    Serial.printf("Running time : %.1f s\r\n", millis() / 1000.0f);
    delay(speed);

    digitalWrite(LED_BUILTIN, HIGH);
    delay(speed);
    digitalWrite(LED_BUILTIN, LOW);
    delay(speed);

    for (int i = 0; i < std::size(colorSequence); i++) {
        rgbStrip.setColor(colorSequence[i][0], colorSequence[i][1], colorSequence[i][2]);
        delay(speed);
    }

    delay(500);
}
