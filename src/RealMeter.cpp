#include <Arduino.h>
#include "RealMeter.h"
#include <HardwareSerial.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "Freenove_WS2812_Lib_for_ESP32.h"
#include <string>
#include <regex>

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
        debug.println("BLE listener connected.");
        deviceConnected = true;
    };
    void onDisconnect(BLEServer *pServer)
    {
        debug.println("BLE listener disconnected.");
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
    debug.println("Debug output initialized (will not respond to input).");

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    rgb.begin();
    rgb.setBrightness(10);
    rgb.setLedColorData(0, 0, 0, 0);
    debug.println("Status leds initialized.");

    setupBLE("ESP32S3_Bluetooth");
    debug.println("BLE broadcast initialized.");

    meter.begin(115200, SERIAL_8N1, RX1, TX1, true);
    // a message should arrive every second
    meter.setTimeout(1500);
    debug.println("Meter input initialized (cannot output to meter).");
}

// 1-0:1.8.1(003020.519*kWh)
const std::regex dayPowerR(R"(1-0:1.8.1\((\d+\.\d+\*kWh)\))");
// 1-0:1.8.2(003080.021*kWh)
const std::regex nightPowerR(R"(1-0:1.8.2\((\d+\.\d+\*kWh)\))");

String regex_match(String data, std::regex pattern)
{
    std::string stdData = data.c_str();
    std::smatch match;
    return std::regex_search(stdData, match, pattern) ? String(match[1].str().c_str()) : "<no match>";
}

void RealMeter::loop()
{
    debug.println("Waiting for reading.");
    rgb.setLedColorData(0, 0, 255, 0);
    rgb.show();

    // ! prefixes the hash at the end of a message
    String meterReading = meter.readStringUntil('!');

    if (meterReading.charAt(meterReading.length() - 1) != '!')
    {
        debug.println("Completely failed to read, trying again.");
        return;
    }

    // the hash is 4 more chars
    while (meter.available() < 4)
    {
        sleep(10);
    }
    for (int i = 0; i < 4; i++)
    {
        meterReading += meter.read();
    }

    debug.println("Reading received, length = " + String(meterReading.length()) + " chars.");
    rgb.setLedColorData(0, 255, 0, 0);
    rgb.show();

    String day = regex_match(meterReading, dayPowerR);
    String night = regex_match(meterReading, nightPowerR);

    String valuesMessage = "day = " + day + ", night = " + night;

    debug.println("Parsed: \"" + valuesMessage + "\".");

    sleep(100); // give time to see the previous light color
    rgb.setLedColorData(0, 0, 0, 255);
    rgb.show();

    long now = millis();
    if (now - lastMsg > 100 && serverCallbacks->isConnected())
    {
        rgb.setLedColorData(0, 0, 0, 255);
        rgb.show();

        pCharacteristic->setValue(valuesMessage.c_str());
        pCharacteristic->notify();

        lastMsg = now;
    }

    sleep(100); // give time to see the previous light color
}
