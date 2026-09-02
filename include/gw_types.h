#ifndef GW_TYPES_H
#define GW_TYPES_H

enum class SystemState {
    BOOT,
    GSM_INIT,
    GSM_WAIT_NETWORK,
    GSM_GPRS_CONNECT,
    RTC_SYNC,
    MQTT_CONNECT,
    RUNNING
};

enum class GprsAttachState {
    IDLE,
    START,
    WAITING,
    SUCCESS,
    FAIL
};

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

struct NetworkStatus {
    int signalStrength;
    bool networkRegistered;
    bool gprsConnected;
    String networkType;
};

#endif // GW_TYPES_H
