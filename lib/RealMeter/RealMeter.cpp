#include <ESP32_LED.h>
#include "RealMeter.h"
#include <HardwareSerial.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "BLE2904.h"
#include <ESP32_RGB.h>
#include "esp_gatt_common_api.h"
#include <string>
#include <regex>

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
#define METER_UART_TIMEOUT 1500

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
        debug.printf("MTU failure: %s\n", mtuError);
    }

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(serverCallbacks);

    BLEService *pService = pServer->createService(SERVICE_UUID);
    // TODO pushed values are still truncated even when mtu=517
    pValues = pService->createCharacteristic(VALUES_UUID_TX, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    // Characteristic User Description
    BLEDescriptor *friendlyName = new BLEDescriptor((uint16_t)0x2901);
    friendlyName->setValue("Meter readings");
    pValues->addDescriptor(friendlyName);

    // Client Characteristic Configuration
    // enable notifications
    pValues->addDescriptor(new BLE2902());

    // Characteristic Presentation Format
    // advertise it as utf8 string
    // https://www.bluetooth.com/specifications/assigned-numbers/
    BLE2904 *format = new BLE2904();
    format->setFormat(BLE2904::FORMAT_UTF8);
    pValues->addDescriptor(format);

    pService->start();
    pServer->getAdvertising()->start();
}

String formatJson(String day, String night)
{
    return "{\"day\": \"" + day + "\", \"night\": \"" + night + "\"}";
}

void RealMeter::init()
{
    debug.begin(115200);
    debug.println("Debug output initialized (will not respond to input).");

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    rgbStrip.setBrightness(10);
    rgbStrip.setColor(0, 0, 0);
    debug.println("Status leds initialized.");

    setupBLE("ESP32-S3 MLE");
    debug.println("BLE broadcast initialized.");

    meter.begin(115200, SERIAL_8N1, -1, -1, true);
    meter.setTimeout(METER_UART_TIMEOUT);
    debug.println("Meter input initialized (cannot output to meter).");

    pValues->setValue(formatJson("000000.NaN*kWh", "000000.NaN*kWh").c_str());
    pValues->notify();
}

// 1-0:1.8.1(003020.519*kWh)
const std::regex dayPowerR(R"(1-0:1.8.1\(0*(\d+\.\d+\*kWh)\))");
// 1-0:1.8.2(003080.021*kWh)
const std::regex nightPowerR(R"(1-0:1.8.2\(0*(\d+\.\d+\*kWh)\))");

String regex_match(String &data, const std::regex &pattern)
{
    std::string stdData = data.c_str();
    std::smatch match;
    return std::regex_search(stdData, match, pattern) && match.size() > 1 ? String(match[1].str().c_str()) : "<no match>";
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
    debug.println("Waiting for telegram.");
    rgbStrip.setColor(0, 255, 0);

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
        debug.printf("Didn't read properly, trying again:\n%s\n", telegram.c_str());
        return;
    }

    // the hash is 4 more hex chars and a crlf
    String hash = readStringUntilWithTimeoutIncludingTerminator(meter, '\n', METER_UART_TIMEOUT);
    telegram += hash;

    debug.printf("Received telegram, %d chars: %s\n", telegram.length(), telegram.c_str());

    rgbStrip.setColor(255, 0, 0);

    String dayPower = regex_match(telegram, dayPowerR);
    String nightPower = regex_match(telegram, nightPowerR);
    String json = formatJson(dayPower, nightPower);

    // TODO using std::string instead of String avoids this mess where sometimes the logged text is garbage
    debug.printf("Parsed day: \"%s\".\n", dayPower.c_str());
    debug.printf("Parsed night: \"%s\".\n", nightPower.c_str());
    debug.printf("Parsed json: \"%s\".\n", json.c_str());

    delay(100); // give time to see the previous light color

    debug.printf("Currently %d BLE clients connected.\n", serverCallbacks->countClients());

    if (millis() - lastMsg < 100)
    {
        debug.println("Skipping broadcast because of short interval.");
        return;
    }

    debug.println("Broadcasting values.");
    rgbStrip.setColor(0, 0, 255);

    pValues->setValue(json.c_str());
    pValues->notify();

    lastMsg = millis();
    delay(100); // give time to see the previous light color
}
