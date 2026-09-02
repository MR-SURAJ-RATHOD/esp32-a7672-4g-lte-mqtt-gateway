#include "system_state.h"

EventGroupHandle_t sysEvents = nullptr;
volatile SystemState s_systemState = SystemState::BOOT;
volatile bool publishRequested = false;

void systemStateInit() {
    sysEvents = xEventGroupCreate();
    s_systemState = SystemState::BOOT;
}

void systemSetState(SystemState state) {
    s_systemState = state;
    if (state == SystemState::RUNNING) {
        xEventGroupSetBits(sysEvents, EVT_MQTT_READY);
    } else {
        xEventGroupClearBits(sysEvents, EVT_MQTT_READY);
    }
}

SystemState systemGetState() {
    return s_systemState;
}

bool systemIsRunning() {
    return s_systemState == SystemState::RUNNING;
}

void systemRequestPublish() {
    publishRequested = true;
}

bool systemTakePublishRequest() {
    if (!publishRequested) return false;
    publishRequested = false;
    return true;
}
