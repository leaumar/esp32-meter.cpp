#include <Arduino.h>
#include "Chatbox.h"
#include <HardwareSerial.h>

// two terminals can send text to each other in realtime, nothing meter specific
//
// usb cable provides power and main serial
// connect isolator (3.3V) to board: tx to pin 15, rx to pin 16
//
// find ports with ~\.platformio\penv\Scripts\platformio.exe device list
// monitor isolator with ~\.platformio\penv\Scripts\platformio.exe device monitor -p comN -b 115200 --echo
// monitor board with ~\.platformio\penv\Scripts\platformio.exe device monitor -p comN -b 115200 --echo
//
// type messages back and forth

HardwareSerial chat(1);

void Chatbox::init()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    Serial.println("ESP32S3 initialization completed! Write something here to send it to the other serial");

    chat.begin(115200, SERIAL_8N1);
    chat.println("ESP32S3 initialization completed! Write something here to send it to the other serial");
}

void Chatbox::loop()
{
    while (chat.available())
    {
        Serial.write(chat.read());
    }
    while (Serial.available())
    {
        chat.write(Serial.read());
    }
}
