#include "rtc_manager.h"
#include "../gsm/gsm_manager.h" // For GSMManager and global 'gsm' object
#include "../utils/logger.h"

// --- Global Instances ---
std::string CurrentDateTime;
RTCManager rtcManager;

// --- Constructor ---
RTCManager::RTCManager() = default;

// --- Begin RTC ---
bool RTCManager::begin() {
    // Internal RTC is always available on ESP32
    // We can just check if time is set to something reasonable ( > 2000 AD)
    time_t now;
    time(&now);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    
    if (timeinfo.tm_year < (2020 - 1900)) {
        Logger::warn("RTC", "Time not set (pre-2020), waiting for sync.");
        rtcInitSuccess = false; // logic: we need sync
        // Initialize to compile time if really needed, but better to wait for GSM
    } else {
        Logger::info("RTC", "Internal RTC time: " + String(ctime(&now)));
        rtcInitSuccess = true;
    }
    
    // Set timezone to UTC
    setenv("TZ", "UTC0", 1);
    tzset();

    update();
    return true;
}

// --- Update RTC ---
void RTCManager::update() {
    time_t now;
    time(&now);
    data.utcTimestamp = formatUTC(now);
    CurrentDateTime = data.utcTimestamp;
}

// --- Set Time ---
void RTCManager::setTime(time_t t) {
    struct timeval tv;
    tv.tv_sec = t;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
    update();
}

// --- Format UTC Time ---
std::string RTCManager::formatUTC(time_t t) {
    struct tm timeinfo;
    gmtime_r(&t, &timeinfo);
    
    char buf[25];
    snprintf(buf, sizeof(buf),
             "%04d-%02d-%02dT%02d:%02d:%02dZ",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return std::string(buf);
}

// --- Sync RTC from GSM ---
bool RTCManager::syncFromGSM() {
    Logger::info("RTC", "Attempting sync from GSM...");
    time_t networkTime = gsm.getNetworkTime();
    
    if (networkTime > 1600000000) { // Valid time check (approx year 2020+)
        setTime(networkTime);
        rtcInitSuccess = true;
        Logger::info("RTC", "RTC Synced from GSM! Time: " + String(ctime(&networkTime)));
        return true;
    } else {
        Logger::warn("RTC", "Failed to get valid time from GSM");
        return false;
    }
}
