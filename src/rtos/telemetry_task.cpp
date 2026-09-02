#include <esp_task_wdt.h>
#include <time.h>
#include "rtos_config.h"
#include "system_state.h"
#include "config/config_manager.h"
#include "utils/logger.h"
#include "rtc/rtc_manager.h"

/** Periodic RTC update + publish interval timer (does not block on MQTT). */
void telemetryTask(void* parameter) {
    Logger::info(TASK_TELEMETRY_NAME, "Task started on core " + String(xPortGetCoreID()));

    unsigned long lastPublishRequest = 0;
    unsigned long lastRtcUpdate = 0;

    for (;;) {
        unsigned long now = millis();

        if (now - lastRtcUpdate >= 1000) {
            lastRtcUpdate = now;
            rtcManager.update();
        }

        EventBits_t bits = xEventGroupGetBits(sysEvents);
        if ((bits & EVT_MQTT_READY) && systemIsRunning()) {
            static unsigned long mqttReadySince = 0;
            if (mqttReadySince == 0) {
                mqttReadySince = now;
            }

            bool intervalElapsed = (lastPublishRequest == 0 || now - lastPublishRequest >= PUBLISH_INTERVAL_MS);
            bool warmupDone = (now - mqttReadySince >= 2000);

            if (intervalElapsed && warmupDone) {
                lastPublishRequest = now;
                Logger::info(TASK_TELEMETRY_NAME, "Publish request (interval=" + String(PUBLISH_INTERVAL_MS) + "ms)");
                systemRequestPublish();
            }
        } else {
            xEventGroupWaitBits(sysEvents, EVT_MQTT_READY, false, true, pdMS_TO_TICKS(500));
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_TELEMETRY_DELAY_MS));
    }
}
