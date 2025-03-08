#include <Arduino.h>
#include "RealMeter.h"
#include <HardwareSerial.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "Freenove_WS2812_Lib_for_ESP32.h"
#include <string>

// the usb cable must not be used because the meter provides power
// connect meter Vcc to 5V, gnd to gnd, pullup to 3.3V, meter inverted tx with pullup to pin 15
// for debugging, connect isolator rx to board tx, gnd to gnd, do not connect Vcc or tx
// find port with C:\Users\leaumar\.platformio\penv\Scripts\platformio.exe device list
// monitor isolator with C:\Users\leaumar\.platformio\penv\Scripts\platformio.exe device monitor -p com7 -b 115200

#define LED_BUILTIN 2
#define LEDS_PIN 48

HardwareSerial &debug = Serial;
HardwareSerial meter(1);

// there's 1 rgb led in the strip and it only has channel 0
Freenove_ESP32_WS2812 rgb = Freenove_ESP32_WS2812(1, LEDS_PIN, 0, TYPE_GRB);

BLECharacteristic *pCharacteristic;
long lastMsg = 0;
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks : public BLEServerCallbacks
{
private:
    bool deviceConnected = false;

public:
    void onConnect(BLEServer *pServer)
    {
        debug.println("BLE listener connected");
        deviceConnected = true;
    };
    void onDisconnect(BLEServer *pServer)
    {
        debug.println("BLE listener disconnected");
        deviceConnected = false;
    }
    bool isConnected() const
    {
        return deviceConnected;
    }
};

MyServerCallbacks *serverCallbacks = new MyServerCallbacks();

void setupBLE(String BLEName)
{
    BLEDevice::init(BLEName.c_str());

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(serverCallbacks);

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    // TODO is this how you do it? multiple descriptors on 1 characteristic, with READ | NOTIFY where READ is just for the friendly name?
    BLEDescriptor *friendlyName = new BLEDescriptor((uint16_t)0x2901);
    friendlyName->setValue("Meter reading");
    pCharacteristic->addDescriptor(friendlyName);
    pCharacteristic->addDescriptor(new BLE2902());

    pService->start();
    pServer->getAdvertising()->start();
}

void RealMeter::init()
{
    debug.begin(115200);
    debug.println("ESP32S3 debug output initialized (will not respond to input)");

    pinMode(LED_BUILTIN, OUTPUT);
    rgb.begin();
    rgb.setBrightness(10);
    debug.println("ESP32S3 message leds initialized");

    setupBLE("ESP32S3_Bluetooth");
    debug.println("ESP32S3 BLE broadcast initialized");

    meter.begin(115200, SERIAL_8N1, RX1, TX1, true);
    debug.println("ESP32S3 meter input initialized (cannot output to meter)");
}

void RealMeter::loop()
{
    if (meter.available())
    {
        debug.println("Incoming reading...\n");
        // digitalWrite(LED_BUILTIN, HIGH);
        rgb.setLedColorData(0, 0, 255, 0);
        rgb.show();

        std::string meterReading = "";
        while (meter.available())
        {
            char c = meter.read();
            meterReading += c;
            debug.write(c);
        }

        // digitalWrite(LED_BUILTIN, LOW);
        rgb.setLedColorData(255, 0, 0, 0);
        rgb.show();
        debug.print("\nReading received, length = ");
        debug.print(meterReading.length());
        debug.println(" chars");

        long now = millis();
        if (now - lastMsg > 100 && serverCallbacks->isConnected())
        {
            rgb.setLedColorData(0, 0, 255, 0);
            rgb.show();

            pCharacteristic->setValue(meterReading);
            pCharacteristic->notify();

            lastMsg = now;
        }
    }
}
