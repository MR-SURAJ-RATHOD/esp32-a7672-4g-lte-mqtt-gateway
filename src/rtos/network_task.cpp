#include "rtos_config.h"
#include "system_state.h"
#include "config/config_manager.h"
#include "utils/logger.h"
#include "gw_config.h"
#include "gsm/gsm_manager.h"
#include "mqtt/mqtt_manager.h"
#include "mqtt/telemetry_mqtt.h"
#include "rtc/rtc_manager.h"
#include <time.h>

static bool rtcSyncLogged = false;
static unsigned long rtcSyncStart = 0;
static bool gsmLedState = false;
static unsigned long gsmLedLastToggle = 0;
static const unsigned long GSM_LED_BLINK_INTERVAL = 500;

void networkTask(void* parameter) {
    Logger::info(TASK_NETWORK_NAME, "Task started on core " + String(xPortGetCoreID()));

    for (;;) {

        gsm.loop();
        mqtt.loop();

        unsigned long now = millis();

        if (gsm.isNetworkReady()) {
            digitalWrite(LED2_PIN, HIGH);
        } else if (now - gsmLedLastToggle >= GSM_LED_BLINK_INTERVAL) {
            gsmLedLastToggle = now;
            gsmLedState = !gsmLedState;
            digitalWrite(LED2_PIN, gsmLedState ? HIGH : LOW);
        }

        SystemState state = systemGetState();

        if (state != SystemState::GSM_INIT && state != SystemState::GSM_WAIT_NETWORK) {
            if (!gsm.isNetworkReady()) {
                Logger::warn(TASK_NETWORK_NAME, "Network registration lost");
                systemSetState(SystemState::GSM_INIT);
                gsm.resetAttachState();
            } else if ((state == SystemState::RTC_SYNC ||
                        state == SystemState::MQTT_CONNECT ||
                        state == SystemState::RUNNING) &&
                       !gsm.isGprsConnected()) {
                Logger::warn(TASK_NETWORK_NAME, "GPRS connection lost");
                systemSetState(SystemState::GSM_INIT);
                gsm.resetAttachState();
            }
        }

        state = systemGetState();

        switch (state) {
            case SystemState::GSM_INIT:
                if (gsm.isReady()) {
                    Logger::info(TASK_NETWORK_NAME, "GSM ready, checking network...");
                    systemSetState(SystemState::GSM_WAIT_NETWORK);
                } else {
                    static unsigned long lastInitRetry = 0;
                    if (now - lastInitRetry > GSM_INIT_RETRY_MS) {
                        lastInitRetry = now;
                        Logger::info(TASK_NETWORK_NAME, "Retrying GSM init...");
                        gsm.initializeModem();
                    }
                }
                break;

            case SystemState::GSM_WAIT_NETWORK:
                if (gsm.isNetworkReady()) {
                    Logger::info(TASK_NETWORK_NAME, "Network registered, attaching GPRS...");
                    systemSetState(SystemState::GSM_GPRS_CONNECT);
                } else {
                    static unsigned long lastNetLog = 0;
                    if (now - lastNetLog > 10000) {
                        lastNetLog = now;
                        int csq = gsm.getSignalStrength();
                        Logger::info(TASK_NETWORK_NAME, "Waiting for network registration, CSQ=" + String(csq));
                    }
                }
                break;

            case SystemState::GSM_GPRS_CONNECT:
                gsm.loop();
                if (gsm.isGprsFullyReady()) {
                    Logger::info(TASK_NETWORK_NAME, "GPRS attached, starting RTC sync...");
                    systemSetState(SystemState::RTC_SYNC);
                    rtcSyncLogged = false;
                    rtcSyncStart = now;
                }
                break;

            case SystemState::RTC_SYNC:
                if (!rtcSyncLogged) {
                    rtcSyncLogged = true;
                    Logger::info(TASK_NETWORK_NAME, "RTC_SYNC state");
                }
                {
                    bool synced = rtcManager.syncFromGSM();
                    if (synced || (now - rtcSyncStart > 10000)) {
                        if (synced) Logger::info(TASK_NETWORK_NAME, "RTC sync success");
                        else Logger::warn(TASK_NETWORK_NAME, "RTC sync timeout, proceeding...");
                        systemSetState(SystemState::MQTT_CONNECT);
                    }
                }
                break;

            case SystemState::MQTT_CONNECT: {
                static bool clientIdSet = false;
                if (!clientIdSet && imei.length() >= 4) {
                    String uniqueClientId = DEVICE_ID + "-" + imei.substring(imei.length() - 4);
                    mqtt.setCredentials(uniqueClientId.c_str(), MQTT_CLIENT_USER.c_str(), MQTT_CLIENT_PASS.c_str());
                    Logger::info(TASK_NETWORK_NAME, "MQTT clientId=" + uniqueClientId);
                    clientIdSet = true;
                }

                if (mqtt.ensureConnected()) {
                    Logger::info(TASK_NETWORK_NAME, "MQTT connected to " + MQTT_BROKER + ":" + String(MQTT_PORT));
                    Logger::info(TASK_NETWORK_NAME, "Publish topic: " + MQTT_METRICS_TOPIC);
                    systemSetState(SystemState::RUNNING);
                }
                break;
            }

            case SystemState::RUNNING: {
                static unsigned long lastMqttKeepAlive = 0;

                if (now - lastMqttKeepAlive >= 30000) {
                    lastMqttKeepAlive = now;
                    if (!mqtt.isConnected()) {
                        mqtt.ensureConnected();
                    }
                }

                if (systemTakePublishRequest()) {
                    if (!mqtt.isConnected()) {
                        Logger::warn(TASK_NETWORK_NAME, "MQTT down before publish, reconnecting...");
                        mqtt.ensureConnected();
                    }

                    if (mqtt.isConnected()) {
                        time_t nowTs = time(nullptr);
                        if (nowTs < 1600000000) {
                            nowTs = millis() / 1000;
                        }
                        telemetryMqtt.publishTelemetry(DEVICE_ID.c_str(), nowTs);
                    } else {
                        Logger::error(TASK_NETWORK_NAME, "Publish skipped — MQTT not connected");
                    }
                }
                break;
            }

            default:
                systemSetState(SystemState::GSM_INIT);
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_NETWORK_DELAY_MS));
    }
}
