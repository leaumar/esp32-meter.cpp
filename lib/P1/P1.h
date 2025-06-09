#pragma once

#include <HardwareSerial.h>

namespace P1 {
    namespace {
        // a telegram is sent every second or so
        const unsigned long timeout = 1500;

        // because serial.readStringUntil doesn't include the terminator, you can't tell if the string is complete or timed out
        std::string readStringUntilWithTimeoutIncludingTerminator(Stream &serial, char terminator) {
            auto startMillis = millis();
            std::string received = "";
            // real message is around 1190 chars
            received.reserve(1300);

            while (millis() - startMillis < timeout) {
                // while-ing might keep it reading forever
                int available = serial.available();
                for (int i = 0; i < available; i++) {
                    char c = serial.read();
                    received += c;

                    if (c == terminator) {
                        received.shrink_to_fit();
                        return received;
                    }
                }
            }

            received.shrink_to_fit();
            return received;
        }
    }

    enum class TelegramState { Empty, Partial, Success };

    struct Telegram {
        TelegramState status;
        std::string value;
    };

    void init(HardwareSerial &meter) {
        meter.begin(115200, SERIAL_8N1, -1, -1, true);
        meter.setTimeout(timeout);
    }

    Telegram awaitTelegram(HardwareSerial &meter) {
        // ! prefixes the hash at the end of a message
        auto telegram = readStringUntilWithTimeoutIncludingTerminator(meter, '!');

        if (telegram.length() == 0) {
            return {TelegramState::Empty, ""};
        }

        if (telegram.back() != '!') {
            return {TelegramState::Partial, telegram};
        }

        // the hash is 4 more hex chars and a crlf
        auto hash = readStringUntilWithTimeoutIncludingTerminator(meter, '\n');
        telegram += hash;
        return {TelegramState::Success, telegram};
    }
}
