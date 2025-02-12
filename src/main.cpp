#include <Arduino.h>

// --------------------------

#define LED_BUILTIN 2

#include "Freenove_WS2812_Lib_for_ESP32.h"

#define LEDS_COUNT 1 // The number of led
#define LEDS_PIN 48  // define the pin connected to the led strip
#define CHANNEL 0    // RMT module channel

Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(LEDS_COUNT, LEDS_PIN, CHANNEL, TYPE_GRB);

int colorSequence[5][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255}, {0, 0, 0}};

#include <HardwareSerial.h>

HardwareSerial debug(1);

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    strip.begin();
    strip.setBrightness(10);

    Serial.begin(115200);
    Serial.println("ESP32S3 initialization completed! Write something here to send it to the other serial");

    debug.begin(115200, SERIAL_8N1, RX1, TX1);
    debug.println("ESP32S3 initialization completed! Write something here to send it to the other serial");
}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);

    // Serial.printf("Running time : %.1f s\r\n", millis() / 1000.0f);
    // C:\Users\leaumar\.platformio\penv\Scripts\platformio.exe device monitor -p com7 -b 115200 --echo
    while (debug.available())
    {
        Serial.write(debug.read());
    }
    while (Serial.available())
    {
        debug.write(Serial.read());
    }
    delay(100);

    for (int j = 0; j < 5; j++)
    {
        for (int i = 0; i < LEDS_COUNT; i++)
        {
            strip.setLedColorData(i, colorSequence[j][0], colorSequence[j][1], colorSequence[j][2]);
            strip.show();
            delay(100);
        }
        delay(100);
    }
}

// --------------------------

// String inputString = "";     // a String to hold incoming data
// bool stringComplete = false; // whether the string is complete

// void setup()
// {
//     Serial.begin(115200);
//     Serial.println(String("\nESP32S3 initialization completed!\r\n") + String("Please input some characters,\r\n") + String("select \"Newline\" below and Enter to send message to ESP32S3. \r\n"));
// }

// void loop()
// {
//     if (Serial.available())
//     {                                // judge whether data has been received
//         char inChar = Serial.read(); // read one character
//         inputString += inChar;
//         if (inChar == '\n')
//         {
//             stringComplete = true;
//         }
//     }
//     if (stringComplete)
//     {
//         Serial.printf("inputString: %s \r\n", inputString);
//         inputString = "";
//         stringComplete = false;
//     }
// }

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

// --------------------------

// #include <HardwareSerial.h>

// HardwareSerial fluvius(1);

// void setup()
// {
//     Serial.begin(115200);
//     fluvius.begin(115200, SERIAL_8N1, RX1, TX1, true);

//     Serial.println("ESP32S3 initialization completed!");
// }

// void loop()
// {
//     if (fluvius.available())
//     {
//         Serial.write(fluvius.read());
//     }
// }
