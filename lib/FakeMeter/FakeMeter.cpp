#include "FakeMeter.h"

#include <ESP32_LED.h>
#include <HardwareSerial.h>

// testing the meter interface by using the isolator with inverter to mimick the meter
//
// usb cable provides power and main serial
// connect meter interface to board: red Vcc to 5V, black gnd to gnd, purple pullup to 3.3V, green inverted tx with pullup to pin 15
// connect isolator (5V) tx to meter interface via inverter: tx and gnd only
//
// find ports with ~\.platformio\penv\Scripts\platformio.exe device list
// monitor isolator with ~\.platformio\penv\Scripts\platformio.exe device monitor -p com7 -b 115200 --echo
//
// type messages into isolator and check echo on main serial

HardwareSerial fakeMeter(1);

void FakeMeter::init() {
    pinMode(ESP_32::LED, OUTPUT);

    Serial.begin(115200);
    Serial.println("ESP32S3 debug output initialized (will not respond to input)");

    fakeMeter.begin(115200, SERIAL_8N1, -1, -1, true);
    Serial.println("ESP32S3 meter input initialized (cannot output to meter)");
}

void FakeMeter::loop() {
    if (fakeMeter.available()) {
        digitalWrite(ESP_32::LED, HIGH);

        while (fakeMeter.available()) {
            Serial.write(fakeMeter.read());
        }

        digitalWrite(ESP_32::LED, LOW);
    }
}
