#include "RealMeter.h"

#include <ArduinoJson.h>
#include <ESP32_BLE.h>
#include <ESP32_LED.h>
#include <ESP32_RGB.h>
#include <HardwareSerial.h>
#include <P1.h>

// connect meter interface to board: red Vcc to 5V, black gnd to gnd, purple pullup to 3.3V, green inverted tx with pullup to pin 15
// find ports with ~\.platformio\penv\Scripts\platformio.exe device list
//
// # real meter attachment
//
// attach the board with meter interface to the meter so it works
//
// the usb cable must not be used because the meter provides power
// for debugging, connect isolator (3.3V) to board: rx to tx, gnd to gnd, do not connect Vcc or tx
// monitor isolator with ~\.platformio\penv\Scripts\platformio.exe device monitor -p comN -b 115200
//
// watch logs streaming in
//
// # simulated meter attachment
//
// we'll run the real code to properly test the whole thing, but it's not exactly the same setup
//
// use the usb cable for power and debugging
// connect isolator (5V) to meter interface via inverter: gnd and tx only
//
// monitor board log with ~\.platformio\penv\Scripts\platformio.exe device monitor -p comN -b 115200
// monitor isolator with ~\.platformio\penv\Scripts\platformio.exe device monitor -p comN -b 115200 --echo
//
// send realistic telegrams from the isolator terminal and watch the board respond

HardwareSerial &debug = Serial;
HardwareSerial meter(1);

ESP_32::BLE::Instance *server = nullptr;
unsigned long lastMsg = 0;

class MyCallbacks : public ESP_32::BLE::Callbacks {
  public:
    void onConnect(ESP_32::BLE::OnConnect onConnect) override {
        debug.printf("BLE listener connected, MTU = %d.\n", onConnect.mtu);
        // server cannot negotiate mtu
    }

    void onDisconnect(ESP_32::BLE::OnDisconnect onDisconnect) override {
        debug.println("BLE listener disconnected.");
    }

    void onMtuChanged(ESP_32::BLE::OnMtu onMtu) override {
        debug.printf("BLE MTU negotiated (by client) to %d bytes.\n", onMtu.mtu);
    }
};

std::string formatJson(std::string day, std::string night) {
    StaticJsonDocument<256> doc;
    doc["day"] = day;
    doc["night"] = night;
    std::string json;
    serializeJson(doc, json);
    return json;
}

void RealMeter::init() {
    debug.begin(115200);
    debug.println("Debug output initialized (will not respond to input).");

    pinMode(ESP_32::LED, OUTPUT);
    digitalWrite(ESP_32::LED, LOW);
    ESP_32::RGB.setColor(0, 0, 0);
    debug.println("Status leds initialized.");

    server = new ESP_32::BLE::Instance(
        std::move(
            ESP_32::BLE::init(
                {"ESP32-S3 MLE", debug, std::make_shared<MyCallbacks>()}, {"727EBBC9-A355-44BA-A81A-46B13689FF59"},
                {"97E7B235-51D3-46D0-B426-899C28BFB13B", "Meter readings"}
            )
        )
    );
    debug.println("BLE broadcast initialized.");

    P1::init(meter);
    debug.println("Meter input initialized (cannot output to meter).");

    server->setValue(formatJson("000000.NaN*kWh", "000000.NaN*kWh"));
}

void RealMeter::loop() {
    debug.println("Waiting for telegram.");
    ESP_32::RGB.setColor(0, 255, 0);

    auto telegram = P1::awaitTelegram(meter);

    if (telegram.status == P1::TelegramState::Empty) {
        debug.println("Nothing received.");
        return;
    }

    if (telegram.status == P1::TelegramState::Partial) {
        debug.printf("Didn't read properly, trying again:\n%s\n", telegram.text.c_str());
        return;
    }

    debug.printf("Received telegram, %d chars: %s\n", telegram.text.length(), telegram.text.c_str());

    ESP_32::RGB.setColor(255, 0, 0);

    auto dayPower = P1::readDayConsumption(telegram);
    auto nightPower = P1::readNightConsumption(telegram);
    auto json = formatJson(dayPower, nightPower);

    debug.printf(
        "Parsed day: \"%s\", night: \"%s\", json: \"%s\".\n", dayPower.c_str(), nightPower.c_str(), json.c_str()
    );

    delay(100); // give time to see the previous light color

    debug.printf("Currently %d BLE clients connected.\n", server->countClients());

    if (millis() - lastMsg < 100) {
        debug.println("Skipping broadcast because of short interval.");
        return;
    }

    debug.println("Broadcasting values.");
    ESP_32::RGB.setColor(0, 0, 255);

    server->setValue(json);

    lastMsg = millis();
    delay(100); // give time to see the previous light color
}
