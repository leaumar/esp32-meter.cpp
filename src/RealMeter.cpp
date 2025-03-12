#include <Arduino.h>
#include "RealMeter.h"
#include <HardwareSerial.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "Freenove_WS2812_Lib_for_ESP32.h"
#include "esp_gatt_common_api.h"
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
#define METER_UART_TIMEOUT 1500

// there's 1 rgb led in the strip and it only has channel 0
Freenove_ESP32_WS2812 rgb = Freenove_ESP32_WS2812(1, LEDS_PIN, 0, TYPE_GRB);

BLECharacteristic *pValues;
unsigned long lastMsg = 0;
#define SERVICE_UUID "727EBBC9-A355-44BA-A81A-46B13689FF59"
#define VALUES_UUID_TX "97E7B235-51D3-46D0-B426-899C28BFB13B"

class MyServerCallbacks : public BLEServerCallbacks
{
private:
    unsigned int clients = 0;

public:
    void onConnect(BLEServer *pServer)
    {
        clients += 1;
        uint16_t connId = pServer->getConnId();
        debug.printf("BLE listener connected, MTU = %d.\n", pServer->getPeerMTU(connId));
        // server cannot negotiate mtu
    }

    void onDisconnect(BLEServer *pServer)
    {
        debug.println("BLE listener disconnected.");
        // prevent going negative just in case
        clients -= clients > 0 ? 1 : 0;

        // "small delay prevents crashes" according to chatgpt
        delay(500);
        pServer->getAdvertising()->start();
        debug.println("Restarted advertising.");
    }

    bool hasClients()
    {
        return clients > 0;
    }

    unsigned int countClients()
    {
        return clients;
    }

    void onMtuChanged(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
    {
        debug.printf("BLE MTU negotiated (by client) to %d bytes.\n", param->mtu.mtu);
    }
};

MyServerCallbacks *serverCallbacks = new MyServerCallbacks();

void setupBLE(const String &BLEName)
{
    BLEDevice::init(BLEName.c_str());
    // subscription-pushed values are truncated to MTU
    esp_err_t mtuError = BLEDevice::setMTU(ESP_GATT_MAX_MTU_SIZE);
    if (mtuError != ESP_OK)
    {
        debug.print("MTU failure: ");
        debug.println(mtuError);
    }

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(serverCallbacks);

    BLEService *pService = pServer->createService(SERVICE_UUID);
    // TODO pushed values are still truncated even when mtu=517
    pValues = pService->createCharacteristic(VALUES_UUID_TX, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    BLEDescriptor *friendlyName = new BLEDescriptor((uint16_t)0x2901);
    friendlyName->setValue("Meter readings");
    pValues->addDescriptor(friendlyName);
    // enable notifications
    pValues->addDescriptor(new BLE2902());

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

    setupBLE("ESP32-S3 MLE");
    debug.println("BLE broadcast initialized.");

    meter.begin(115200, SERIAL_8N1, RX1, TX1, true);
    meter.setTimeout(METER_UART_TIMEOUT);
    debug.println("Meter input initialized (cannot output to meter).");
}

// 1-0:1.8.1(003020.519*kWh)
const std::regex dayPowerR(R"(1-0:1.8.1\(0*(\d+\.\d+\*kWh)\))");
// 1-0:1.8.2(003080.021*kWh)
const std::regex nightPowerR(R"(1-0:1.8.2\(0*(\d+\.\d+\*kWh)\))");

String regex_match(String &data, const std::regex &pattern)
{
    std::string stdData = data.c_str();
    std::smatch match;
    return std::regex_search(stdData, match, pattern) ? String(match[1].str().c_str()) : "<no match>";
}

// because serial.readStringUntil doesn't include the terminator, you can't tell if the string is complete or timed out
String readStringUntilWithTimeoutIncludingTerminator(HardwareSerial &serial, char terminator, unsigned long timeout)
{
    unsigned long startMillis = millis();
    String received = "";

    while (millis() - startMillis < timeout)
    {
        // while-ing might keep it reading forever
        int available = serial.available();
        for (int i = 0; i < available; i++)
        {
            char c = serial.read();
            received += c;

            if (c == terminator)
            {
                return received;
            }
        }
    }

    return received;
}

void RealMeter::loop()
{
    pValues->setValue("abcdefghijklmnopqrstuvwxyz0123456789 lorem ipsum dolor sit amet");
    pValues->notify();

    debug.println("Waiting for telegram.");
    rgb.setLedColorData(0, 0, 255, 0);
    rgb.show();

    // ! prefixes the hash at the end of a message
    // a message should arrive every second
    String telegram = readStringUntilWithTimeoutIncludingTerminator(meter, '!', METER_UART_TIMEOUT);

    if (telegram.length() == 0)
    {
        debug.println("Nothing received.");
        return;
    }

    if (telegram.charAt(telegram.length() - 1) != '!')
    {
        debug.println("Didn't read properly, trying again:");
        debug.println(telegram);
        return;
    }

    // the hash is 4 more hex chars and a crlf
    String hash = readStringUntilWithTimeoutIncludingTerminator(meter, '\n', METER_UART_TIMEOUT);
    telegram += hash;

    debug.print("Received telegram, ");
    debug.print(String(telegram.length()));
    debug.println(" chars:");
    debug.println(telegram);

    rgb.setLedColorData(0, 255, 0, 0);
    rgb.show();

    String dayPower = regex_match(telegram, dayPowerR);
    String nightPower = regex_match(telegram, nightPowerR);

    String valuesMessage = "day = " + dayPower + ", night = " + nightPower;

    debug.print("Parsed: \"");
    debug.print(valuesMessage);
    debug.println("\".");

    delay(100); // give time to see the previous light color

    debug.print("Currently ");
    debug.print(serverCallbacks->countClients());
    debug.println(" BLE clients connected.");

    unsigned long now = millis();
    if (now - lastMsg > 100 && serverCallbacks->hasClients())
    {
        debug.println("Broadcasting values.");
        rgb.setLedColorData(0, 0, 0, 255);
        rgb.show();

        pValues->setValue(valuesMessage.c_str());
        pValues->notify();

        lastMsg = now;
        delay(100); // give time to see the previous light color
    }

    debug.println("End of loop.");
}
