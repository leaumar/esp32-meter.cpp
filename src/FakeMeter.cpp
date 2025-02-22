#include <Arduino.h>
#include "FakeMeter.h"
#include <HardwareSerial.h>

// usb cable provides power and main serial
// connect isolator tx with inverter to pin 15, gnd to gnd, do not connect Vcc or tx
// find port with C:\Users\leaumar\.platformio\penv\Scripts\platformio.exe device list
// monitor isolator with C:\Users\leaumar\.platformio\penv\Scripts\platformio.exe device monitor -p com7 -b 115200 --echo
// type messages into isolator and check echo on main serial

#define LED_BUILTIN 2

HardwareSerial fakeMeter(1);

void FakeMeter::init()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    Serial.println("ESP32S3 debug output initialized (will not respond to input)");

    fakeMeter.begin(115200, SERIAL_8N1, RX1, TX1, true);
    Serial.println("ESP32S3 meter input initialized (cannot output to meter)");
}

void FakeMeter::loop()
{
    if (fakeMeter.available())
    {
        digitalWrite(LED_BUILTIN, HIGH);

        while (fakeMeter.available())
        {
            Serial.write(fakeMeter.read());
        }

        digitalWrite(LED_BUILTIN, LOW);
    }
}
