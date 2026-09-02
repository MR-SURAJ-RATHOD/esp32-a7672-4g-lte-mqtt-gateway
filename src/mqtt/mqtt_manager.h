#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <TinyGsmClient.h>
#include "../include/gw_config.h"

class MQTTManager {
public:
    MQTTManager();

    void begin(TinyGsmClient& client, SemaphoreHandle_t mutex);
    void loop();

    bool isReady() const { return ready; }
    bool isConnected();

    bool ensureConnected();

    bool publish(const char* topic, const JsonDocument& doc);
    bool publishRaw(const char* topic, const char* payload, size_t len);

    void setCallback(void (*callback)(char*, byte*, unsigned int));
    void setBroker(const char* broker, uint16_t port);
    void setCredentials(const char* id, const char* username, const char* password);

private:
    PubSubClient mqtt;
    TinyGsmClient* gsmClient;
    bool ready;
    bool connected;
    unsigned long lastReconnectAttempt;

    String clientId;
    String user;
    String pass;

    void (*userCallback)(char*, byte*, unsigned int) = nullptr;

    bool connectInternal();
    bool publishViaGsm(const char* topic, const uint8_t* payload, size_t len, bool retain);
    static size_t encodeMqttRemainingLength(uint8_t* out, size_t value);

    SemaphoreHandle_t _mutex;
};

extern MQTTManager mqtt;

#endif // MQTT_MANAGER_H
