#include "telemetry_mqtt.h"
#include "mqtt_manager.h"
#include "../utils/logger.h"
#include "config/config_manager.h"
#include <stdio.h>

TelemetryMqtt telemetryMqtt;

void TelemetryMqtt::begin(const char* topic) {
    strncpy(_topic, topic, sizeof(_topic) - 1);
    _topic[sizeof(_topic) - 1] = '\0';
    Logger::info("Telemetry", "Publish topic: " + String(_topic));
}

bool TelemetryMqtt::publishTelemetry(const char* deviceId, time_t unixTime) {
    static unsigned long lastPublishMs = 0;
    unsigned long nowMs = millis();
    if (lastPublishMs > 0) {
        unsigned long gapMs = nowMs - lastPublishMs;
        Logger::info("Telemetry",
                     "Interval=" + String(gapMs) + "ms target=" + String(PUBLISH_INTERVAL_MS) + "ms");
    }
    lastPublishMs = nowMs;

    metricsBuilder.readPayload(_payload);

    DynamicJsonDocument doc(4096);

    /**
     * CUSTOMIZE: MQTT JSON envelope for your automation backend.
     *
     * Typical automation publish shape:
     *   {
     *     "ID": "GW-PLANT-01",
     *     "DT": "1735689600",
     *     "DATA": {
     *       "temp_c": "26.5",
     *       "door_open": "0",
     *       "motor_run": "1"
     *     }
     *   }
     *
     * Add site / tenant keys if your platform needs them:
     *   doc["tenant_id"] = TENANT_ID;
     *   doc["asset_id"] = ASSET_ID;
     *   doc["site"] = "factory_line_2";
     */
    doc["ID"] = deviceId;

    char dtBuf[16];
    snprintf(dtBuf, sizeof(dtBuf), "%ld", (long)unixTime);
    doc["DT"] = dtBuf;

    // doc["tenant_id"] = TENANT_ID;
    // doc["asset_id"] = ASSET_ID;

    JsonObject data = doc.createNestedObject("DATA");
    metricsBuilder.fillJsonPayload(data, _payload);

    size_t jsonLen = measureJson(doc);
    Logger::info("Telemetry",
                 "Publishing ID=" + String(deviceId) + " DT=" + String((long)unixTime) +
                     " bytes=" + String(jsonLen));

    bool ok = mqtt.publish(_topic, doc);
    if (!ok) {
        Logger::error("Telemetry", "Publish FAILED bytes=" + String(jsonLen));
    }
    return ok;
}
