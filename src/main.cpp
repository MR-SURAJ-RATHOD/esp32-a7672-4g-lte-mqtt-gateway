/*
 * ESP32 + A7672 / SIM7672 — 4G LTE MQTT gateway for automation telemetry
 *
 * Use case: remote sites without Wi-Fi — publish real-time sensor / I/O data
 * over cellular (temperature, levels, digital status, Modbus devices, etc.).
 *
 * CONFIG:  data/config.local.env  →  pio run -t uploadfs
 * PAYLOAD: src/metrics/metrics_builder.*  +  src/mqtt/telemetry_mqtt.cpp
 */

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "rtos_config.h"
#include "rtos/system_state.h"
#include "config/config_manager.h"
#include "utils/logger.h"
#include "gw_config.h"
#include "gsm/gsm_manager.h"
#include "mqtt/mqtt_manager.h"
#include "mqtt/telemetry_mqtt.h"
#include "rtc/rtc_manager.h"

GSMManager gsm;

SemaphoreHandle_t sysMutex = nullptr;
String imei;

TaskHandle_t networkTaskHandle = nullptr;
TaskHandle_t telemetryTaskHandle = nullptr;

void networkTask(void* parameter);
void telemetryTask(void* parameter);

void setup() {
    esp_task_wdt_init(WDT_TIMEOUT_SEC, false);
    sysMutex = xSemaphoreCreateRecursiveMutex();
    systemStateInit();

    Logger::begin();
    Logger::info("Main", "=== ESP32 4G LTE A7672 MQTT Gateway ===");
    Logger::info("Main", "Firmware Version: " + String(FIRMWARE_VERSION));
    Logger::info("Main", "FreeRTOS dual-task mode enabled");

    if (!config.begin()) {
        Logger::warn("Main", "SPIFFS config not fully loaded — check data/config.local.env");
    }
    config.printConfigFile();

    pinMode(LED2_PIN, OUTPUT);
    digitalWrite(LED2_PIN, LOW);

    gsm.begin(sysMutex);
    mqtt.begin(gsm.getClient(), sysMutex);
    mqtt.setBroker(MQTT_BROKER.c_str(), MQTT_PORT);
    mqtt.setCredentials(DEVICE_ID.c_str(), MQTT_CLIENT_USER.c_str(), MQTT_CLIENT_PASS.c_str());

    telemetryMqtt.begin(MQTT_METRICS_TOPIC.c_str());

    rtcManager.begin();

    Logger::info("Main", "MQTT clientId(base)=" + DEVICE_ID + " user=" + MQTT_CLIENT_USER);

    systemSetState(SystemState::GSM_INIT);

    xTaskCreatePinnedToCore(
        networkTask,
        TASK_NETWORK_NAME,
        TASK_NETWORK_STACK,
        nullptr,
        TASK_NETWORK_PRIO,
        &networkTaskHandle,
        TASK_NETWORK_CORE);

    xTaskCreatePinnedToCore(
        telemetryTask,
        TASK_TELEMETRY_NAME,
        TASK_TELEMETRY_STACK,
        nullptr,
        TASK_TELEMETRY_PRIO,
        &telemetryTaskHandle,
        TASK_TELEMETRY_CORE);

    Logger::info("Main", "RTOS tasks created — Network core " + String(TASK_NETWORK_CORE) +
                 ", Telemetry core " + String(TASK_TELEMETRY_CORE));
}

void loop() {
    vTaskDelete(NULL);
}
