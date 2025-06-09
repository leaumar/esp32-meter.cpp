#pragma once

#include <HardwareSerial.h>
#include <regex>

namespace P1 {
    enum class TelegramState { Empty, Partial, Success };

    struct Telegram {
        TelegramState status;
        std::string text;
    };

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

        std::string regex_match(const Telegram &telegram, const std::regex &pattern) {
            if (telegram.status != TelegramState::Success) {
                throw std::runtime_error("Do not try to parse incomplete telegrams.");
            }

            std::smatch match;
            bool matches = std::regex_search(telegram.text, match, pattern) && match.size() > 1;

            // since we're parsing our own telegram and not a random string, we can assume this won't happen
            if (!matches) {
                throw std::runtime_error("Could not parse value line from telegram.");
            }

            return match[1].str();
        }
    }

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

    // 3020.519*kWh
    std::string readDayConsumption(const Telegram &telegram) {
        // 1-0:1.8.1(003020.519*kWh)
        const std::regex regex(R"(1-0:1.8.1\(0*(\d+\.\d+\*kWh)\))");
        return regex_match(telegram, regex);
    }

    // 3080.021*kWh
    std::string readNightConsumption(const Telegram &telegram) {
        // 1-0:1.8.2(003080.021*kWh)
        const std::regex regex(R"(1-0:1.8.2\(0*(\d+\.\d+\*kWh)\))");
        return regex_match(telegram, regex);
    }
}
