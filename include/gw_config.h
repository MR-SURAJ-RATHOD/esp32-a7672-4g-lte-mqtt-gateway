#ifndef GW_CONFIG_H
#define GW_CONFIG_H

#include <Arduino.h>
#include <stdint.h>

/** Firmware version string (shown on serial boot log). */
#define FIRMWARE_VERSION "1.0.0"

#ifndef TINY_GSM_MODEM_SIM7600
#define TINY_GSM_MODEM_SIM7600
#endif

// --- Serial debug (USB) ---
#define SerialMon Serial
#define BAUD_RATE 115200

// --- Modem UART (ESP32 <-> A7672 / SIM7672 on VVM501 board) ---
// CHANGE if your board uses different pins (see your module schematic).
#define SerialAT  Serial1
#define GSM_BAUD 115200
#define GSM_RX_PIN 27   // ESP32 RX  <- modem TX
#define GSM_TX_PIN 26   // ESP32 TX  -> modem RX
#define GSM_POWER_PIN 4 // Modem power key / enable

#define GSM_BOOT_WAIT_MS 30000
#define GSM_POWER_PULSE_MS 2000
#define GSM_INIT_RETRY_MS 15000

// --- Status LED (built-in GPIO2 on many ESP32 dev boards) ---
#define LED_PIN 2
#define LED2_PIN 2

// --- SPIFFS config file paths (upload via PlatformIO data/ folder) ---
#define CONFIG_FILE_PATH "/config.env"
#define CONFIG_LOCAL_FILE_PATH "/config.local.env"

/**
 * Cellular APN (optional).
 * Leave empty in config to use automatic APN detection from SIM IMSI/ICCID.
 * Set APN=your.apn in config.local.env to force a specific operator APN.
 */
extern char APN[64];
extern char USER[32];
extern char PASS[32];

#endif // GW_CONFIG_H
