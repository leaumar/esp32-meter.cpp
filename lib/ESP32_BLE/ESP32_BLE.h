#include <Arduino.h>
#include <BLE2902.h>
#include <BLE2904.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Stream.h>
#include <esp_gatt_common_api.h>
#include <memory>

// it's said that the BLE stack can only handle 1 client connection at a time, no concurrent connections
// abstraction goal:
// - 1 server with 1 service having 1 property
//    - server: name, mtu, callbacks
//    - service: id
//    - property (utf8 string): id, name
// - max 1 client, always advertising when free
// - can be shut down and destroyed cleanly
// - not exposing the BLE stack API

namespace ESP_32 {
    namespace BLE {
        struct OnConnect {
            uint32_t clients;
            uint16_t connId;
            uint16_t mtu;
        };

        struct OnDisconnect {
            uint32_t clients;
        };

        struct OnMtu {
            uint32_t clients;
            uint16_t mtu;
        };

        class Callbacks {
          public:
            virtual void onConnect(OnConnect onConnect) = 0;
            virtual void onDisconnect(OnDisconnect onDisconnect) = 0;
            virtual void onMtuChanged(OnMtu onMtu) = 0;
            virtual ~Callbacks() = default;
        };

        class _CallbackSimplifier : public BLEServerCallbacks {
          private:
            std::shared_ptr<ESP_32::BLE::Callbacks> callbacks;

          public:
            _CallbackSimplifier(std::shared_ptr<ESP_32::BLE::Callbacks> cb) : callbacks(std::move(cb)) {}

            void onConnect(BLEServer *pServer) {
                auto connId = pServer->getConnId();
                auto mtu = pServer->getPeerMTU(connId);
                auto clients = pServer->getConnectedCount();
                ESP_32::BLE::OnConnect on = {clients, connId, mtu};
                // server cannot negotiate mtu
                callbacks->onConnect(on);
            }

            void onDisconnect(BLEServer *pServer) {
                auto clients = pServer->getConnectedCount();
                ESP_32::BLE::OnDisconnect on = {clients};
                callbacks->onDisconnect(on);

                // "small delay prevents crashes" according to chatgpt
                delay(500);
                // connection causes advertising to stop, so restart it
                pServer->getAdvertising()->start();
            }

            void onMtuChanged(BLEServer *pServer, esp_ble_gatts_cb_param_t *param) {
                auto mtu = param->mtu.mtu;
                auto clients = pServer->getConnectedCount();
                ESP_32::BLE::OnMtu on = {clients, mtu};
                callbacks->onMtuChanged(on);
            }
        };

        class Instance {
          private:
            std::shared_ptr<BLEServer> server;
            std::shared_ptr<BLEService> service;
            std::shared_ptr<BLECharacteristic> characteristic;
            std::shared_ptr<_CallbackSimplifier> simplifier;
            std::vector<std::shared_ptr<BLEDescriptor>> descriptors;

            // TODO destructor?

          public:
            Instance(
                std::shared_ptr<BLEServer> s, std::shared_ptr<BLEService> sv, std::shared_ptr<BLECharacteristic> c,
                std::shared_ptr<_CallbackSimplifier> cb, std::vector<std::shared_ptr<BLEDescriptor>> d
            )
                : server(std::move(s)), service(std::move(sv)), characteristic(std::move(c)), simplifier(std::move(cb)),
                  descriptors(std::move(d)) {}

            void setValue(const std::string &value) {
                characteristic->setValue(value);
                characteristic->notify();
            }

            uint32_t countClients() {
                return server->getConnectedCount();
            }
        };

        struct ServerProps {
            std::string name;
            Stream &debug;
            std::shared_ptr<Callbacks> callbacks;
        };

        struct ServiceProps {
            std::string id;
        };

        struct CharacteristicProps {
            std::string id;
            std::string name;
        };

        Instance init(ServerProps serverProps, ServiceProps serviceProps, CharacteristicProps characteristicProps) {
            BLEDevice::init(serverProps.name);

            // subscription-pushed values are truncated to MTU
            // must be called after init but before start
            // TODO pushed values are still truncated even when mtu=517
            esp_err_t mtuError = BLEDevice::setMTU(ESP_GATT_MAX_MTU_SIZE);
            if (mtuError != ESP_OK) {
                serverProps.debug.printf("MTU failure: %s\n", mtuError);
            }

            std::shared_ptr<_CallbackSimplifier> simplifier(new _CallbackSimplifier(serverProps.callbacks));

            std::shared_ptr<BLEServer> server(BLEDevice::createServer());
            server->setCallbacks(simplifier.get());

            std::shared_ptr<BLEService> service(server->createService(serviceProps.id));

            std::shared_ptr<BLECharacteristic> characteristic(service->createCharacteristic(
                characteristicProps.id, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
            ));

            // Characteristic User Description
            std::shared_ptr<BLEDescriptor> friendlyName(new BLEDescriptor((uint16_t)0x2901));
            friendlyName->setValue(characteristicProps.name);
            characteristic->addDescriptor(friendlyName.get());

            // Client Characteristic Configuration
            // enable notifications
            std::shared_ptr<BLEDescriptor> notifying(new BLE2902());
            characteristic->addDescriptor(notifying.get());

            // Characteristic Presentation Format
            // advertise it as utf8 string
            // https://www.bluetooth.com/specifications/assigned-numbers/
            std::shared_ptr<BLE2904> format(new BLE2904());
            format->setFormat(BLE2904::FORMAT_UTF8);
            characteristic->addDescriptor(format.get());

            std::vector<std::shared_ptr<BLEDescriptor>> descriptors = {friendlyName, notifying, format};

            service->start();
            server->getAdvertising()->start();

            return Instance(server, service, characteristic, simplifier, descriptors);
        }
    }
}
