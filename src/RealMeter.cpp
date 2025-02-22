#include <Arduino.h>
#include "RealMeter.h"
#include <HardwareSerial.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "Freenove_WS2812_Lib_for_ESP32.h"

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

        String meterReading = "";
        while (meter.available())
        {
            char c = meter.read();
            meterReading += c;
            debug.write(c);
        }

        // digitalWrite(LED_BUILTIN, LOW);
        rgb.setLedColorData(255, 0, 0, 0);
        rgb.show();
        debug.println("\nReading received, length=" + String(meterReading.length()) + " chars");

        long now = millis();
        if (now - lastMsg > 100 && serverCallbacks->isConnected())
        {
            rgb.setLedColorData(0, 0, 255, 0);
            rgb.show();

            const char *newValue = meterReading.c_str();
            pCharacteristic->setValue(newValue);
            pCharacteristic->notify();

            lastMsg = now;
        }
    }
}

// --------------------------

// #include "BLEDevice.h"
// #include "BLEServer.h"
// #include "BLEUtils.h"
// #include "BLE2902.h"

// BLECharacteristic *pCharacteristic;
// bool deviceConnected = false;
// uint8_t txValue = 0;
// long lastMsg = 0;
// String rxload = "Test\n";

// #define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
// #define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
// #define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// class MyServerCallbacks : public BLEServerCallbacks
// {
//     void onConnect(BLEServer *pServer)
//     {
//         deviceConnected = true;
//     };
//     void onDisconnect(BLEServer *pServer)
//     {
//         deviceConnected = false;
//         // pServer->getAdvertising()->start();  //Reopen the pServer and wait for the connection.
//     }
// };

// class MyCallbacks : public BLECharacteristicCallbacks
// {
//     void onWrite(BLECharacteristic *pCharacteristic)
//     {
//         String rxValue = String(pCharacteristic->getValue().c_str());
//         if (rxValue.length() > 0)
//         {
//             rxload = "";
//             for (int i = 0; i < rxValue.length(); i++)
//             {
//                 rxload += (char)rxValue[i];
//             }
//         }
//     }
// };

// void setupBLE(String BLEName)
// {
//     const char *ble_name = BLEName.c_str();
//     BLEDevice::init(ble_name);
//     BLEServer *pServer = BLEDevice::createServer();
//     pServer->setCallbacks(new MyServerCallbacks());
//     BLEService *pService = pServer->createService(SERVICE_UUID);
//     pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
//     pCharacteristic->addDescriptor(new BLE2902());
//     BLECharacteristic *pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
//     pCharacteristic->setCallbacks(new MyCallbacks());
//     pService->start();
//     pServer->getAdvertising()->start();
//     Serial.println("Waiting a client connection to notify...");
// }

// void setup()
// {
//     Serial.begin(115200);
//     setupBLE("ESP32S3_Bluetooth");
// }

// void loop()
// {
//     long now = millis();
//     if (now - lastMsg > 100)
//     {
//         if (deviceConnected && rxload.length() > 0)
//         {
//             Serial.println(rxload);
//             rxload = "";
//         }
//         if (Serial.available() > 0)
//         {
//             String str = Serial.readString();
//             const char *newValue = str.c_str();
//             pCharacteristic->setValue(newValue);
//             pCharacteristic->notify();
//         }
//         lastMsg = now;
//     }
// }

// --------------------------

// #include "BLEDevice.h"
// #include "BLEServer.h"
// #include "BLEUtils.h"
// #include "BLE2902.h"
// #include "String.h"

// BLECharacteristic *pCharacteristic;
// bool deviceConnected = false;
// uint8_t txValue = 0;
// long lastMsg = 0;
// char rxload[20];

// #define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
// #define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
// #define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
// #define LED 2

// class MyServerCallbacks : public BLEServerCallbacks
// {
//     void onConnect(BLEServer *pServer)
//     {
//         deviceConnected = true;
//     };
//     void onDisconnect(BLEServer *pServer)
//     {
//         deviceConnected = false;
//         // pServer->getAdvertising()->start();  //Reopen the pServer and wait for the connection.
//     }
// };

// class MyCallbacks : public BLECharacteristicCallbacks
// {
//     void onWrite(BLECharacteristic *pCharacteristic)
//     {
//         String rxValue = pCharacteristic->getValue();
//         if (rxValue.length() > 0)
//         {
//             for (int i = 0; i < 20; i++)
//             {
//                 rxload[i] = 0;
//             }
//             for (int i = 0; i < rxValue.length(); i++)
//             {
//                 rxload[i] = (char)rxValue[i];
//             }
//         }
//     }
// };

// void setupBLE(String BLEName)
// {
//     const char *ble_name = BLEName.c_str();
//     BLEDevice::init(ble_name);
//     BLEServer *pServer = BLEDevice::createServer();
//     pServer->setCallbacks(new MyServerCallbacks());
//     BLEService *pService = pServer->createService(SERVICE_UUID);
//     pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
//     pCharacteristic->addDescriptor(new BLE2902());
//     BLECharacteristic *pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
//     pCharacteristic->setCallbacks(new MyCallbacks());
//     pService->start();
//     pServer->getAdvertising()->start();
//     Serial.println("Waiting a client connection to notify...");
// }

// void setup()
// {
//     pinMode(LED, OUTPUT);
//     setupBLE("ESP32S3_Bluetooth");
//     Serial.begin(115200);
//     Serial.println("\nThe device started, now you can pair it with Bluetooth!");
// }

// void loop()
// {
//     long now = millis();
//     if (now - lastMsg > 100)
//     {
//         if (deviceConnected && strlen(rxload) > 0)
//         {
//             if (strncmp(rxload, "led_on", 6) == 0)
//             {
//                 digitalWrite(LED, HIGH);
//             }
//             if (strncmp(rxload, "led_off", 7) == 0)
//             {
//                 digitalWrite(LED, LOW);
//             }
//             Serial.println(rxload);
//             memset(rxload, 0, sizeof(rxload));
//         }
//         lastMsg = now;
//     }
// }
