#include "metrics_builder.h"

#include <stdio.h>

MetricsBuilder metricsBuilder;

void MetricsBuilder::putMetricStr(JsonObject obj, const char* key, float value) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%.4f", value);
    obj[key] = buf;
}

void MetricsBuilder::putMetricBool(JsonObject obj, const char* key, bool value) {
    obj[key] = value ? "1" : "0";
}

void MetricsBuilder::readPayload(TelemetryPayload& payload) {
    /**
     * CUSTOMIZE: read your automation sensors here (real-time sampling).
     *
     * GPIO digital input (door open):
     *   payload.door_open = digitalRead(DOOR_SENSOR_PIN) == HIGH;
     *
     * ADC tank level (0–3.3 V → 0–100 %):
     *   int raw = analogRead(LEVEL_ADC_PIN);
     *   payload.tank_level_pct = (raw / 4095.0f) * 100.0f;
     *
     * I2C temperature (use your driver or Wire read):
     *   payload.temperature_c = myTempSensor.readCelsius();
     *
     * Modbus RTU holding register (RS-485):
     *   payload.flow_lpm = modbus.readFloat(REG_FLOW);
     *
     * Pulse counter (flow totalizer):
     *   payload.total_pulse_count = pulseCounter.getCount();
     *
     * Stub below keeps firmware buildable without sensors wired.
     */
    payload.field_a = 0.0f;
    payload.field_b = 0.0f;
}

void MetricsBuilder::fillJsonPayload(JsonObject dataObject, const TelemetryPayload& payload) {
    /**
     * CUSTOMIZE: JSON keys for your SCADA / cloud / Node-RED / Grafana pipeline.
     *
     * putMetricStr(dataObject, "temp_c", payload.temperature_c);
     * putMetricStr(dataObject, "humidity_pct", payload.humidity_pct);
     * putMetricStr(dataObject, "tank_level_pct", payload.tank_level_pct);
     * putMetricBool(dataObject, "door_open", payload.door_open);
     * putMetricBool(dataObject, "motor_run", payload.motor_running);
     *
     * Site grouping (optional):
     *   JsonObject plant = dataObject.createNestedObject("plant_a");
     *   plant["line1_temp"] = String(payload.temperature_c, 2);
     */

    putMetricStr(dataObject, "field_a", payload.field_a);
    putMetricStr(dataObject, "field_b", payload.field_b);
}
