
---

# RTC Manager Module

**ESP32 Internal RTC + GSM Time Sync + UTC Formatting**

This module provides a **lightweight RTC manager** for ESP32-based IoT devices.
It handles:

*  Internal RTC access (ESP32 `time.h`)
*  UTC timestamp formatting
*  Synchronization with GSM network time
*  Global timestamp storage (`CurrentDateTime`)
*  Safe time setting and updates

Designed for IoT devices that require **accurate UTC timestamps** for telemetry, commands, or logging.

---

#  Architecture Overview

```
GSMManager (gsm) → RTCManager → Global CurrentDateTime → Service Modules
```

* `RTCManager` abstracts the internal RTC and provides a standard UTC timestamp
* Supports GSM network time sync
* Can format any `time_t` to ISO8601 UTC

---

#  Module Structure

```
rtc_manager.h
rtc_manager.cpp
```

* **rtc_manager.h** – Header with class declaration and global instances
* **rtc_manager.cpp** – Implementation with GSM sync and formatting logic

---

#  RTCManager Class

Provides methods for initializing, updating, and reading the RTC.

---

##  Global Instances

```cpp
extern RTCManager rtcManager;
extern std::string CurrentDateTime;  // Current UTC timestamp
```

`CurrentDateTime` is updated on each `update()` call and can be used globally.

---

##  Initialization

```cpp
rtcManager.begin();
```

* Checks if ESP32 internal RTC has valid time (> year 2020)
* Sets timezone to UTC
* Updates `CurrentDateTime`
* If time is invalid, `rtcInitSuccess` is `false`

---

##  Update Timestamp

```cpp
rtcManager.update();
```

* Updates `CurrentDateTime` from the internal RTC
* Can be called periodically to refresh timestamps

---

##  Set RTC Time

```cpp
rtcManager.setTime(time_t t);
```

* Sets the ESP32 RTC to the given `time_t`
* Updates `CurrentDateTime`
* Useful after GSM sync or manual time adjustments

---

##  Sync RTC from GSM

```cpp
bool ok = rtcManager.syncFromGSM();
```

* Queries GSM network time via global `gsm` object
* Sets RTC if valid (network time > 2020)
* Updates `CurrentDateTime` and `rtcInitSuccess`

---

##  Get Current Data

```cpp
const RTCData& data = rtcManager.getData();
std::string utc = data.utcTimestamp;
```

* Returns UTC timestamp in **ISO8601 format**: `YYYY-MM-DDTHH:MM:SSZ`

---

##  Format Any `time_t` to UTC

```cpp
std::string utcStr = RTCManager::formatUTC(time_tValue);
```

* Static helper to convert any `time_t` to ISO8601 UTC string

**Example:**

```cpp
time_t now;
time(&now);
std::string timestamp = RTCManager::formatUTC(now);
// "2026-03-02T11:45:23Z"
```

---

#  Dependencies

* ESP32 Arduino Core
* `<time.h>` / `<sys/time.h>`
* Standard C++ library `<string>`
* Global `gsm` object for GSM network time

---

#  Integration Example

```cpp
#include "rtc_manager.h"

void setup() {
    Serial.begin(115200);

    rtcManager.begin(); // Initialize internal RTC
    
    // Attempt GSM sync
    if (!rtcManager.rtcInitSuccess) {
        rtcManager.syncFromGSM();
    }

    Serial.println("Current UTC: " + String(CurrentDateTime.c_str()));
}

void loop() {
    // Update timestamp periodically
    rtcManager.update();
    delay(1000);
}
```

---

#  Design Principles

* Non-blocking, lightweight update
* Internal RTC used as primary source
* GSM sync only if internal time is invalid
* Provides globally accessible ISO8601 UTC timestamp
* Safe for multi-tasking in FreeRTOS

---

#  Usage Scenarios

* Timestamping MQTT messages (`ServiceMqtt`)
* Logging events with UTC
* Telemetry metrics for cloud storage
* Scheduling tasks in UTC
* Time-based session management

---
