#ifndef METRICS_BUILDER_H
#define METRICS_BUILDER_H

#include <ArduinoJson.h>

/**
 * =============================================================================
 * CUSTOMIZE YOUR MQTT PAYLOAD HERE (automation / IoT telemetry)
 * =============================================================================
 *
 * This module samples your field hardware and builds the JSON body for MQTT.
 * There is NO fixed industry schema — you choose fields for YOUR automation
 * project (building, factory, agriculture, water, energy monitoring, etc.).
 *
 * Steps:
 *   1. Edit TelemetryPayload — add variables for each sensor or PLC point.
 *   2. readPayload() — read GPIO, ADC, I2C, Modbus RTU, pulse counters, etc.
 *   3. fillJsonPayload() — map values to JSON keys your dashboard expects.
 *
 * Example automation use cases (pick what you need):
 *   - Temperature / humidity (DHT, SHT, BME over I2C)
 *   - Tank level (ultrasonic, 4–20 mA into ADC, Modbus level transmitter)
 *   - Door / limit switch (digital GPIO input)
 *   - Motor run status, RPM, or hour counter (digital input + pulse)
 *   - Flow rate / totalizer (pulse meter on GPIO interrupt)
 *   - Relay / valve feedback (digital output readback)
 *   - Room occupancy, smoke, gas alarm (digital or analog)
 *   - RS-485 Modbus energy meter, VFD, or remote I/O block
 *
 * Example JSON (see telemetry_mqtt.cpp for root keys ID / DT / DATA):
 *   {
 *     "ID": "GW-PLANT-01",
 *     "DT": "1735689600",
 *     "DATA": {
 *       "temp_c": "26.50",
 *       "humidity_pct": "58.00",
 *       "tank_level_pct": "72.30",
 *       "door_open": "0",
 *       "motor_run": "1"
 *     }
 *   }
 *
 * Real-time path: TelemetryTask triggers every PUBLISH_INTERVAL_MS →
 * readPayload() runs → MQTT publish. Tune interval in config.local.env.
 * =============================================================================
 */

struct TelemetryPayload {
    // Placeholder fields — rename/replace for your automation project.
    float field_a = 0.0f;
    float field_b = 0.0f;

    // --- Example fields (uncomment and use in readPayload / fillJsonPayload) ---
    // float temperature_c = 0.0f;      // e.g. I2C temp/humidity sensor
    // float humidity_pct = 0.0f;
    // float tank_level_pct = 0.0f;   // e.g. ADC or Modbus level
    // bool  door_open = false;       // e.g. GPIO limit switch
    // bool  motor_running = false;   // e.g. auxiliary contact
    // float flow_lpm = 0.0f;         // e.g. pulse counter scaled to L/min
    // uint32_t total_pulse_count = 0;
};

class MetricsBuilder {
public:
    /** Sample sensors / PLC inputs into payload. Called before each MQTT publish. */
    void readPayload(TelemetryPayload& payload);

    /** Write payload into nested JSON object (under "DATA" in telemetry_mqtt.cpp). */
    void fillJsonPayload(JsonObject dataObject, const TelemetryPayload& payload);

private:
    static void putMetricStr(JsonObject obj, const char* key, float value);
    static void putMetricBool(JsonObject obj, const char* key, bool value);
};

extern MetricsBuilder metricsBuilder;

#endif // METRICS_BUILDER_H
