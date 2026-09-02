#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include "gw_config.h"

/**
 * Runtime configuration loaded from SPIFFS:
 *   /config.env          — template keys (YOUR_* placeholders)
 *   /config.local.env    — optional overrides (recommended for secrets)
 *
 * Edit files under data/ then:  pio run -t uploadfs
 */

extern String DEVICE_ID;
extern String ASSET_ID;
extern String TENANT_ID;

extern String MQTT_BROKER;
extern uint16_t MQTT_PORT;
extern String MQTT_CLIENT_USER;
extern String MQTT_CLIENT_PASS;
extern String MQTT_METRICS_TOPIC;

extern uint32_t NETWORK_REGISTER_TIMEOUT_MS;
extern uint32_t GPRS_ATTACH_TIMEOUT_MS;
extern uint32_t MQTT_RECONNECT_DELAY_MS;
extern uint32_t PUBLISH_INTERVAL_MS;

/** Set cellular APN credentials (called when parsing config.env). */
void setApn(const String& apn);
void setApnUser(const String& user);
void setApnPass(const String& pass);

class ConfigManager {
public:
    bool begin(const char* path = CONFIG_FILE_PATH);
    bool reload(const char* path = CONFIG_FILE_PATH);
    void printConfigFile();

private:
    const char* _path;
    void parseLine(const String& line);
    void assignValue(const String& key, const String& value);
};

extern ConfigManager config;

#endif // CONFIG_MANAGER_H
