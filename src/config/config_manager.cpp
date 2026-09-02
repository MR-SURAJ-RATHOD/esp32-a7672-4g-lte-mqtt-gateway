#include "config_manager.h"
#include "gw_config.h"
#include "utils/logger.h"

// Compiled defaults when SPIFFS config is missing.
// CUSTOMIZE: replace YOUR_* placeholders or rely on data/config.local.env on SPIFFS.
String DEVICE_ID = "YOUR_DEVICE_ID";
String ASSET_ID = "YOUR_ASSET_ID";
String TENANT_ID = "YOUR_TENANT_ID";

String MQTT_BROKER = "YOUR_MQTT_BROKER_HOST";
uint16_t MQTT_PORT = 1883;
String MQTT_CLIENT_USER = "YOUR_MQTT_USERNAME";
String MQTT_CLIENT_PASS = "YOUR_MQTT_PASSWORD";
String MQTT_METRICS_TOPIC = "YOUR_MQTT_PUBLISH_TOPIC";

uint32_t NETWORK_REGISTER_TIMEOUT_MS = 60000;
uint32_t GPRS_ATTACH_TIMEOUT_MS = 30000;
uint32_t MQTT_RECONNECT_DELAY_MS = 5000;
uint32_t PUBLISH_INTERVAL_MS = 60000;

// Empty APN => GSMManager uses automatic APN detection from SIM IMSI/ICCID.
char APN[64] = "";
char USER[32] = "";
char PASS[32] = "";

void setApn(const String& apn) {
    apn.toCharArray(APN, sizeof(APN));
}

void setApnUser(const String& user) {
    user.toCharArray(USER, sizeof(USER));
}

void setApnPass(const String& pass) {
    pass.toCharArray(PASS, sizeof(PASS));
}

bool ConfigManager::begin(const char* path) {
    _path = path;

    if (!SPIFFS.begin(true)) {
        Logger::info("Config", "SPIFFS mount failed -> using compiled defaults");
        return false;
    }

    bool loaded = false;
    if (SPIFFS.exists(_path)) {
        loaded = reload(_path);
    } else {
        Logger::info("Config", String(_path) + " missing on SPIFFS");
    }

    // Optional local overrides (recommended for secrets on your machine).
    if (SPIFFS.exists(CONFIG_LOCAL_FILE_PATH)) {
        loaded = reload(CONFIG_LOCAL_FILE_PATH) || loaded;
    } else {
        Logger::info("Config",
                     "Tip: create data/config.local.env and run pio run -t uploadfs");
    }

    return loaded;
}

bool ConfigManager::reload(const char* path) {
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) {
        Logger::info("Config", String("Open failed: ") + path);
        return false;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        parseLine(line);
    }

    file.close();
    Logger::info("Config", String("Loaded ") + path);
    return true;
}

void ConfigManager::parseLine(const String& line) {
    if (line.isEmpty() || line.startsWith("#"))
        return;

    int idx = line.indexOf('=');
    if (idx < 0)
        return;

    String key = line.substring(0, idx);
    String value = line.substring(idx + 1);
    key.trim();
    value.trim();
    assignValue(key, value);
}

void ConfigManager::assignValue(const String& key, const String& value) {
    if (key == "DEVICE_ID")
        DEVICE_ID = value;
    else if (key == "ASSET_ID")
        ASSET_ID = value;
    else if (key == "TENANT_ID")
        TENANT_ID = value;
    else if (key == "MQTT_BROKER")
        MQTT_BROKER = value;
    else if (key == "MQTT_PORT")
        MQTT_PORT = value.toInt();
    else if (key == "MQTT_CLIENT_USER")
        MQTT_CLIENT_USER = value;
    else if (key == "MQTT_CLIENT_PASS")
        MQTT_CLIENT_PASS = value;
    else if (key == "MQTT_METRICS_TOPIC")
        MQTT_METRICS_TOPIC = value;
    else if (key == "APN")
        setApn(value);
    else if (key == "APN_USER")
        setApnUser(value);
    else if (key == "APN_PASS")
        setApnPass(value);
    else if (key == "PUBLISH_INTERVAL_MS")
        PUBLISH_INTERVAL_MS = value.toInt();
    else if (key == "NETWORK_REGISTER_TIMEOUT_MS")
        NETWORK_REGISTER_TIMEOUT_MS = value.toInt();
    else if (key == "GPRS_ATTACH_TIMEOUT_MS")
        GPRS_ATTACH_TIMEOUT_MS = value.toInt();
    else if (key == "MQTT_RECONNECT_DELAY_MS")
        MQTT_RECONNECT_DELAY_MS = value.toInt();
}

void ConfigManager::printConfigFile() {
    Logger::info("Config", "DEVICE_ID=" + DEVICE_ID);
    Logger::info("Config", "ASSET_ID=" + ASSET_ID);
    Logger::info("Config", "TENANT_ID=" + TENANT_ID);
    Logger::info("Config",
                 "MQTT_BROKER=" + MQTT_BROKER + ":" + String(MQTT_PORT));
    Logger::info("Config", "MQTT_CLIENT_USER=" + MQTT_CLIENT_USER);
    Logger::info("Config", "MQTT_METRICS_TOPIC=" + MQTT_METRICS_TOPIC);
    Logger::info("Config", "PUBLISH_INTERVAL_MS=" + String(PUBLISH_INTERVAL_MS));
    Logger::info("Config",
                 String("APN=") + (strlen(APN) ? APN : "(auto-detect)"));
}

ConfigManager config;
