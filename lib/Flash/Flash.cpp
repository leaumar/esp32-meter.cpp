#include "Flash.h"

#include "polyfill.h"

#include <ESP32_LED.h>
#include <ESP32_RGB.h>

// usb cable provides power and main serial

int colorSequence[5][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255}, {0, 0, 0}};

void Flash::init() {
    pinMode(ESP_32::LED, OUTPUT);

    Serial.begin(115200);
    Serial.println("ESP32S3 initialization completed!");
}

int waitTime = 200;

void Flash::loop() {
    delay(waitTime);

    digitalWrite(ESP_32::LED, HIGH);
    delay(waitTime);
    digitalWrite(ESP_32::LED, LOW);
    delay(waitTime);

    float randomBrightness = static_cast<float>(rand()) / RAND_MAX;
    Serial.printf("Brightness: %.1f\r\n", randomBrightness);

    for (int i = 0; i < std::size(colorSequence); i++) {
        int r = colorSequence[i][0];
        int g = colorSequence[i][1];
        int b = colorSequence[i][2];
        ESP_32::RGB.setColor(
            static_cast<int>(r * randomBrightness), static_cast<int>(g * randomBrightness),
            static_cast<int>(b * randomBrightness)
        );
        delay(waitTime);
    }

    delay(waitTime * 2);
}
