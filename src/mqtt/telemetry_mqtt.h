#ifndef TELEMETRY_MQTT_H
#define TELEMETRY_MQTT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "metrics/metrics_builder.h"

/**
 * Builds and publishes the periodic MQTT telemetry message.
 *
 * CUSTOMIZE publishTelemetry() JSON envelope (root keys, tenant/asset fields)
 * and MetricsBuilder for the nested DATA object.
 */
class TelemetryMqtt {
public:
    void begin(const char* topic);
    bool publishTelemetry(const char* deviceId, time_t unixTime);
    TelemetryPayload& payload() { return _payload; }

private:
    char _topic[128];
    TelemetryPayload _payload;
};

extern TelemetryMqtt telemetryMqtt;

#endif // TELEMETRY_MQTT_H
