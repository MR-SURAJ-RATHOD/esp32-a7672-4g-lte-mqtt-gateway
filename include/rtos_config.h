#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

/**
 * FreeRTOS task layout (ESP32 dual-core).
 *
 * NetworkTask  — GSM bring-up, GPRS, RTC sync, MQTT connect, publish on request
 * TelemetryTask — RTC tick, publish interval timer, sets publish request flag
 *
 * CHANGE stack sizes / priorities only if you add heavy work inside tasks.
 */

#define TASK_NETWORK_NAME     "NetworkTask"
#define TASK_TELEMETRY_NAME   "TelemetryTask"

#define TASK_NETWORK_STACK    12288
#define TASK_TELEMETRY_STACK  12288

#define TASK_NETWORK_PRIO     2
#define TASK_TELEMETRY_PRIO   1

#define TASK_NETWORK_CORE     0
#define TASK_TELEMETRY_CORE   1

#define TASK_NETWORK_DELAY_MS   50
#define TASK_TELEMETRY_DELAY_MS 20

#define WDT_TIMEOUT_SEC       120

/** Set when state machine reaches RUNNING (MQTT connected). */
#define EVT_MQTT_READY        BIT0

#endif // RTOS_CONFIG_H
