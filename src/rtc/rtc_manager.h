#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include <string>

// --- RTC Data Struct ---
struct RTCData {
    std::string utcTimestamp;  // "YYYY-MM-DDTHH:MM:SSZ" (UTC format)
};

// --- RTC Manager Class ---
class RTCManager {
public:
    RTCManager();

    bool begin();
    void update();

    void setTime(time_t t);
    bool syncFromGSM(); // Uses global GSMManager

    [[nodiscard]] const RTCData& getData() const noexcept { return data; }

    [[nodiscard]] static std::string formatUTC(time_t t);
    
    bool rtcInitSuccess = false;

private:
    RTCData data;
};

// --- Global Instances ---
extern RTCManager rtcManager;
extern std::string CurrentDateTime;

#endif // RTC_MANAGER_H
