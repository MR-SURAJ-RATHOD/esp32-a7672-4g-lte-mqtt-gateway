#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include "gw_types.h"
#include "rtos_config.h"

extern EventGroupHandle_t sysEvents;
extern volatile SystemState s_systemState;
extern volatile bool publishRequested;

void systemStateInit();
void systemSetState(SystemState state);
SystemState systemGetState();
bool systemIsRunning();
void systemRequestPublish();
bool systemTakePublishRequest();

#endif // SYSTEM_STATE_H
