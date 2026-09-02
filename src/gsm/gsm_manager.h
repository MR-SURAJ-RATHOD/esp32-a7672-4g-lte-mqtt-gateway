#ifndef GSM_MANAGER_H
#define GSM_MANAGER_H

#include <Arduino.h>
#include <TinyGsmClient.h>
#include "../include/gw_config.h"
#include "../include/gw_types.h"
#include "config/config_manager.h"

class GSMManager {
public:
    GSMManager();
    void begin(SemaphoreHandle_t mutex);
    bool initializeModem(); // New robust init method
    void loop();
    bool isReady() const { return ready; }
    
    bool isNetworkReady();
    bool isGprsConnected();
     
    bool isGprsFullyReady();  // Checks IP address assignment
    void resetAttachState();
    String getLocalIP();
    TinyGsmClient& getClient();
    int getSignalStrength();
    String getNetworkType();
    NetworkStatus getStatus();
    /** Used by APN trial callback to test GPRS with given APN */
    bool tryGprsConnect(const char* apn, const char* user, const char* pass);
    
    // RTC Sync
    time_t getNetworkTime();
    
    void forceGprsReconnect();
    bool checkGprsReal();
    bool updateImei();

private:
    SemaphoreHandle_t _mutex; // Shared mutex for SerialAT
    String apnName;
    String apnUser;
    String apnPass;
    TinyGsm modem;
    TinyGsmClient client;
    bool ready;
    bool networkRegistered;
    bool gprsAttached;
    unsigned long lastCheck;
    unsigned long networkWaitStart;
    
    GprsAttachState gprsAttachState;
    unsigned long gprsStateStartTime;
    unsigned long gprsLastTry;
    uint8_t gprsDisconnectDebounce = 0;
    uint8_t resetCount = 0; // Track reset attempts for escalation

    void resetModem();
    void checkNetworkStatus();
    void attemptGprsAttach();
    void configureHardware();
    void powerOnModem();
    bool scanBaudRate();
    bool waitForAtReady(unsigned long timeoutMs);
    void logSerialBootMessages(unsigned long windowMs);
    uint8_t initAttemptCount = 0;
    
};

extern GSMManager gsm;
extern String imei;

#endif // GSM_MANAGER_H
