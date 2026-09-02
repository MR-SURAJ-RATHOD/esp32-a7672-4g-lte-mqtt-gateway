#include "mqtt_manager.h"
#include "../utils/logger.h"
#include "config/config_manager.h"
#include "gsm/gsm_manager.h"
#include <esp_task_wdt.h>

static constexpr size_t MQTT_PUBLISH_BUFFER_SIZE = 4096;
static constexpr size_t GSM_WRITE_CHUNK = 128;
static char s_mqttPayloadBuffer[MQTT_PUBLISH_BUFFER_SIZE];

MQTTManager::MQTTManager()
    : gsmClient(nullptr),
      ready(false),
      connected(false),
      lastReconnectAttempt(0),
      _mutex(nullptr) {}

void MQTTManager::begin(TinyGsmClient& client, SemaphoreHandle_t mutex) {
    _mutex = mutex;
    gsmClient = &client;

    mqtt.setBufferSize(MQTT_PUBLISH_BUFFER_SIZE);
    mqtt.setClient(client);
    mqtt.setKeepAlive(60);
    mqtt.setSocketTimeout(20);

    ready = true;
    Logger::info("MQTT", "MQTT buffer size=" + String(MQTT_PUBLISH_BUFFER_SIZE));
}

void MQTTManager::loop() {
    if (!ready) return;

    if (_mutex && xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        mqtt.loop();
        connected = mqtt.connected();
        xSemaphoreGiveRecursive(_mutex);
    }
}

bool MQTTManager::isConnected() {
    bool c = false;
    if (_mutex && xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        c = mqtt.connected();
        connected = c;
        xSemaphoreGiveRecursive(_mutex);
    }
    return c;
}

bool MQTTManager::ensureConnected() {
    if (!ready) return false;

    if (isConnected()) {
        return true;
    }

    if (_mutex && xSemaphoreTakeRecursive(_mutex, portMAX_DELAY) == pdTRUE) {
        bool outcome = false;

        if (mqtt.connected()) {
            connected = true;
            outcome = true;
        } else {
            unsigned long now = millis();
            if (now - lastReconnectAttempt >= MQTT_RECONNECT_DELAY_MS) {
                lastReconnectAttempt = now;
                outcome = connectInternal();
            }
        }
        xSemaphoreGiveRecursive(_mutex);
        return outcome;
    }
    return false;
}

bool MQTTManager::connectInternal() {
    if (clientId.isEmpty()) {
        Logger::error("MQTT", "Client ID not set");
        return false;
    }

    if (!gsm.isGprsConnected()) {
        Logger::warn("MQTT", "GPRS not connected, skipping MQTT connect");
        return false;
    }

    Logger::info("MQTT", "Connecting to broker...");
    mqtt.setSocketTimeout(25);

    bool ok;
    if (user.isEmpty() || pass.isEmpty()) {
        ok = mqtt.connect(clientId.c_str());
    } else {
        ok = mqtt.connect(clientId.c_str(), user.c_str(), pass.c_str());
    }

    if (!ok) {
        int state = mqtt.state();
        Logger::error("MQTT", "Connect failed, rc=" + String(state));

        if (state == -2 || state == -3) {
            Logger::warn("MQTT", "Network error detected. Forcing GPRS reconnect...");
            gsm.forceGprsReconnect();
            if (gsmClient) gsmClient->stop();
        }

        connected = false;
        return false;
    }

    connected = true;
    Logger::info("MQTT", "MQTT connected");

    if (userCallback) {
        mqtt.setCallback(userCallback);
    }

    return true;
}

size_t MQTTManager::encodeMqttRemainingLength(uint8_t* out, size_t value) {
    size_t i = 0;
    do {
        uint8_t encoded = value % 128;
        value /= 128;
        if (value > 0) {
            encoded |= 0x80;
        }
        out[i++] = encoded;
    } while (value > 0);
    return i;
}

bool MQTTManager::publishViaGsm(const char* topic, const uint8_t* payload, size_t len, bool retain) {
    if (!gsmClient || topic == nullptr || payload == nullptr || len == 0) {
        return false;
    }

    size_t topicLen = strlen(topic);
    size_t remaining = 2 + topicLen + len;
    if (remaining > 4096) {
        Logger::error("MQTT", "Publish packet too large");
        return false;
    }

    uint8_t fixedHeader[6];
    fixedHeader[0] = 0x30 | (retain ? 0x01 : 0x00);
    size_t rlBytes = encodeMqttRemainingLength(fixedHeader + 1, remaining);

    size_t written = gsmClient->write(fixedHeader, 1 + rlBytes);
    if (written != 1 + rlBytes) {
        Logger::warn("MQTT", "GSM header write failed");
        return false;
    }

    uint8_t topicLenBytes[2] = {
        (uint8_t)((topicLen >> 8) & 0xFF),
        (uint8_t)(topicLen & 0xFF)
    };
    if (gsmClient->write(topicLenBytes, 2) != 2) {
        Logger::warn("MQTT", "GSM topic length write failed");
        return false;
    }
    if (gsmClient->write((const uint8_t*)topic, topicLen) != topicLen) {
        Logger::warn("MQTT", "GSM topic write failed");
        return false;
    }

    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > GSM_WRITE_CHUNK) {
            chunk = GSM_WRITE_CHUNK;
        }

        size_t sent = gsmClient->write(payload + offset, chunk);
        if (sent != chunk) {
            Logger::warn("MQTT", "GSM payload write failed at " + String(offset));
            return false;
        }

        offset += chunk;
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    return true;
}

bool MQTTManager::publish(const char* topic, const JsonDocument& doc) {
    if (!ready || topic == nullptr) return false;

    size_t len = measureJson(doc);
    if (len == 0 || len >= MQTT_PUBLISH_BUFFER_SIZE - 64) {
        Logger::error("MQTT", "JSON size invalid: " + String(len));
        return false;
    }

    size_t written = serializeJson(doc, s_mqttPayloadBuffer, MQTT_PUBLISH_BUFFER_SIZE);
    if (written == 0) {
        Logger::error("MQTT", "JSON serialize failed");
        return false;
    }

    return publishRaw(topic, s_mqttPayloadBuffer, written);
}

bool MQTTManager::publishRaw(const char* topic, const char* payload, size_t len) {
    if (!ready || topic == nullptr || payload == nullptr || len == 0) return false;

    bool ret = false;
    if (_mutex && xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
        if (!mqtt.connected()) {
            Logger::error("MQTT", "Publish skipped — not connected, state=" + String(mqtt.state()));
        } else {
            Logger::info("MQTT", "Publishing to " + String(topic) + " len=" + String(len) + " retain=0");

            for (int attempt = 1; attempt <= 3; attempt++) {
                vTaskDelay(pdMS_TO_TICKS(10));

                ret = publishViaGsm(topic, (const uint8_t*)payload, len, false);
                if (ret) {
                    vTaskDelay(pdMS_TO_TICKS(200));
                    mqtt.loop();
                    break;
                }

                Logger::warn("MQTT", "GSM publish attempt " + String(attempt) + "/3 failed");
                vTaskDelay(pdMS_TO_TICKS(400));
                mqtt.loop();
            }

            connected = mqtt.connected();
            if (ret) {
                Logger::info("MQTT", "Published OK to " + String(topic) +
                             " connected=" + String(connected ? 1 : 0) +
                             " preview=" + String(payload).substring(0, 80));
            } else {
                Logger::error("MQTT", "Publish FAILED topic=" + String(topic) +
                               " rc=" + String(mqtt.state()) + " connected=" + String(connected ? 1 : 0));
            }
        }
        xSemaphoreGiveRecursive(_mutex);
    } else {
        Logger::warn("MQTT", "Publish skipped — mutex busy");
    }
    return ret;
}

void MQTTManager::setCallback(void (*callback)(char*, byte*, unsigned int)) {
    userCallback = callback;
    mqtt.setCallback(userCallback);
}

void MQTTManager::setBroker(const char* broker, uint16_t port) {
    mqtt.setServer(broker, port);
    if (userCallback) {
        mqtt.setCallback(userCallback);
    }
    Logger::info("MQTT", "Broker set: " + String(broker) + ":" + String(port));
}

void MQTTManager::setCredentials(const char* id, const char* username, const char* password) {
    clientId = id;
    user = username;
    pass = password;
}

MQTTManager mqtt;
