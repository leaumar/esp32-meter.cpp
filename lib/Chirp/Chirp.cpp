#include "Chirp.h"

#include "polyfill.h"

#include <ArduinoJson.h>
#include <ESP32_BLE.h>
#include <ESP32_LED.h>
#include <HardwareSerial.h>

// usb cable provides power and main serial

class MyCallbacks : public ESP_32::BLE::Callbacks {
  public:
    void onConnect(ESP_32::BLE::OnConnect onConnect) override {
        Serial.printf("BLE listener connected, MTU = %d.\n", onConnect.mtu);
    }

    void onDisconnect(ESP_32::BLE::OnDisconnect onDisconnect) override {
        Serial.println("BLE listener disconnected.");
    }

    void onMtuChanged(ESP_32::BLE::OnMtu onMtu) override {
        Serial.printf("BLE MTU negotiated (by client) to %d bytes.\n", onMtu.mtu);
    }
};

ESP_32::BLE::Instance *server = nullptr;

std::string formatJson(std::string day, std::string night) {
    StaticJsonDocument<256> doc;
    doc["day"] = day;
    doc["night"] = night;
    std::string json;
    serializeJson(doc, json);
    return json;
}

void Chirp::init() {
    pinMode(ESP_32::LED, OUTPUT);

    auto s = ESP_32::BLE::init(
        {"ESP32-S3 MLE", Serial, std::make_shared<MyCallbacks>()}, {"727EBBC9-A355-44BA-A81A-46B13689FF59"},
        {"97E7B235-51D3-46D0-B426-899C28BFB13B", "Meter readings"}
    );

    server = new ESP_32::BLE::Instance(std::move(s));

    Serial.begin(115200);
    Serial.println("ESP32S3 initialization completed!");
}

void Chirp::loop() {
    int random = rand();
    auto json = formatJson(std::to_string(random), std::to_string(random));
    Serial.printf("Value: %d, clients: %d, json: %s\r\n", random, server->countClients(), json.c_str());

    digitalWrite(ESP_32::LED, HIGH);
    server->setValue(json);
    delay(100);
    digitalWrite(ESP_32::LED, LOW);

    delay(1000);
}
