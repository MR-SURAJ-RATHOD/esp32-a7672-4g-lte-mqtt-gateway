#include "gsm_manager.h"
#include "apn_detector.h"
#include "gw_types.h"
#include "../utils/logger.h"
#include <esp_task_wdt.h>

static GSMManager* s_gsmForTrial = nullptr;
static bool trialApnCallback(const char* apn, const char* user, const char* pass) {
    if (s_gsmForTrial) return s_gsmForTrial->tryGprsConnect(apn, user, pass);
    return false;
}

static String sanitizeIp(const String& raw) {
    int idx = raw.indexOf("10.");
    if (idx < 0) idx = raw.indexOf("192.");
    if (idx < 0) idx = raw.indexOf("172.");
    if (idx >= 0) {
        String ip = raw.substring(idx);
        ip.trim();
        int space = ip.indexOf(' ');
        if (space > 0) ip = ip.substring(0, space);
        return ip;
    }
    return raw;
}

GSMManager::GSMManager()
    : modem(SerialAT),
      client(modem),
      ready(false),
      networkRegistered(false),
      gprsAttached(false),
      lastCheck(0),
      networkWaitStart(0),
      gprsAttachState(GprsAttachState::IDLE),
      gprsStateStartTime(0),
      gprsLastTry(0),
      _mutex(nullptr),
      gprsDisconnectDebounce(0) {}

void GSMManager::configureHardware() {
    pinMode(GSM_POWER_PIN, OUTPUT);
    digitalWrite(GSM_POWER_PIN, LOW);

    SerialAT.setRxBufferSize(4096);
    SerialAT.setTxBufferSize(1024);
    SerialAT.begin(GSM_BAUD, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
    delay(300);
}

void GSMManager::begin(SemaphoreHandle_t mutex) {
    _mutex = mutex;

    Logger::info("GSM", "Configuring GSM — RX=" + String(GSM_RX_PIN) +
                 " TX=" + String(GSM_TX_PIN) + " PWR=" + String(GSM_POWER_PIN) +
                 " baud=" + String(GSM_BAUD));

    configureHardware();
    Logger::info("GSM", "Hardware configured — modem init deferred to NetworkTask");
}

void GSMManager::logSerialBootMessages(unsigned long windowMs) {
    unsigned long start = millis();
    while (millis() - start < windowMs) {
        while (SerialAT.available()) {
            String line = SerialAT.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                Logger::info("GSM", "MODEM: " + line);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
        esp_task_wdt_reset();
    }
}

bool GSMManager::waitForAtReady(unsigned long timeoutMs) {
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        logSerialBootMessages(200);
        if (modem.testAT(1000)) {
            Logger::info("GSM", "AT ready in " + String(millis() - start) + " ms");
            return true;
        }
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    return false;
}

bool GSMManager::scanBaudRate() {
    static const uint32_t bauds[] = {115200, 9600, 57600, 230400, 460800};

    Logger::info("GSM", "Scanning baud rates...");

    for (uint32_t baud : bauds) {
        SerialAT.end();
        vTaskDelay(pdMS_TO_TICKS(150));
        SerialAT.begin(baud, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
        SerialAT.flush();
        vTaskDelay(pdMS_TO_TICKS(300));

        modem.sendAT(GF(""));
        modem.waitResponse(100);

        if (modem.testAT(1500)) {
            Logger::info("GSM", "Modem responded at " + String(baud));

            if (baud != GSM_BAUD) {
                Logger::info("GSM", "Switching modem to " + String(GSM_BAUD));
                modem.sendAT(GF("+IPR="), GSM_BAUD);
                modem.waitResponse(2000L);
                modem.sendAT(GF("&W"));
                modem.waitResponse(2000L);
                SerialAT.end();
                vTaskDelay(pdMS_TO_TICKS(200));
                SerialAT.begin(GSM_BAUD, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
                vTaskDelay(pdMS_TO_TICKS(500));
                if (!modem.testAT(2000)) {
                    Logger::warn("GSM", "Baud switch verify failed");
                }
            }
            return true;
        }
    }

    Logger::warn("GSM", "No baud rate response");
    return false;
}

void GSMManager::powerOnModem() {
    initAttemptCount++;

    if (initAttemptCount % 3 == 0) {
        Logger::info("GSM", "Power sequence: HOLD HIGH (alternate)");
        digitalWrite(GSM_POWER_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(300));
        digitalWrite(GSM_POWER_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(3000));
        digitalWrite(GSM_POWER_PIN, LOW);
    } else {
        Logger::info("GSM", "Power sequence: PULSE HIGH");
        digitalWrite(GSM_POWER_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(300));
        digitalWrite(GSM_POWER_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(GSM_POWER_PULSE_MS));
        digitalWrite(GSM_POWER_PIN, LOW);
    }

    Logger::info("GSM", "Waiting for boot (max " + String(GSM_BOOT_WAIT_MS / 1000) + "s)...");
    waitForAtReady(GSM_BOOT_WAIT_MS);
}

bool GSMManager::updateImei() {
    if (!_mutex) return false;

    if (xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(2000))) {
        String id = modem.getIMEI();
        xSemaphoreGiveRecursive(_mutex);
        if (id.length() > 0) {
            imei = id;
            Logger::info("GSM", "IMEI: " + imei);
            return true;
        }
    }
    return false;
}

bool GSMManager::initializeModem() {
    if (!_mutex) return false;

    if (!xSemaphoreTakeRecursive(_mutex, portMAX_DELAY)) return false;

    esp_task_wdt_reset();
    Logger::info("GSM", "=== Modem init attempt " + String(initAttemptCount + 1) + " ===");

    modem.sendAT(GF(""));
    modem.waitResponse(100);

    bool responding = false;
    for (int i = 0; i < 5; i++) {
        if (modem.testAT(1500)) {
            responding = true;
            break;
        }
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(400));
    }

    if (!responding) {
        Logger::info("GSM", "No AT response — running power-on sequence");
        xSemaphoreGiveRecursive(_mutex);
        powerOnModem();
        if (!xSemaphoreTakeRecursive(_mutex, portMAX_DELAY)) return false;

        if (!scanBaudRate() && !modem.testAT(3000)) {
            Logger::error("GSM", "Modem not responding after power + baud scan");
            ready = false;
            xSemaphoreGiveRecursive(_mutex);
            return false;
        }
    } else {
        Logger::info("GSM", "Modem already responding");
    }

    Logger::info("GSM", "Initializing modem (TinyGSM init)...");
    bool ok = false;
    for (int i = 0; i < 5; i++) {
        esp_task_wdt_reset();
        if (modem.init()) {
            ok = true;
            break;
        }
        Logger::warn("GSM", "TinyGSM init failed, retry " + String(i + 1) + "/5");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    if (!ok) {
        Logger::warn("GSM", "TinyGSM init failed — trying AT+CFUN=1 restart");
        modem.sendAT(GF("+CFUN=0"));
        modem.waitResponse(5000L);
        vTaskDelay(pdMS_TO_TICKS(2000));
        modem.sendAT(GF("+CFUN=1"));
        modem.waitResponse(15000L);
        vTaskDelay(pdMS_TO_TICKS(3000));
        ok = modem.init();
    }

    if (ok) {
        Logger::info("GSM", "Configuring LTE...");
        modem.sendAT(GF("+CNMP=2"));
        modem.waitResponse(3000L);
        modem.sendAT(GF("+CMNB=3"));
        if (modem.waitResponse(3000L) != 1) {
            modem.sendAT(GF("+CMNB=1"));
            modem.waitResponse(3000L);
        }
        modem.sendAT(GF("+CFUN=1"));
        if (modem.waitResponse(15000L) != 1) {
            Logger::warn("GSM", "CFUN=1 timeout");
        }

        modem.sendAT(GF("+CPIN?"));
        String cpinResp;
        if (modem.waitResponse(3000L, cpinResp) == 1) {
            Logger::info("GSM", "SIM: " + cpinResp);
        }

        ApnResult_t apnResult = {};
        String imsi = modem.getIMSI();
        if (imsi.length() > 0) {
            apnResult = apnDetector.detectFromIMSI(imsi.c_str());
        } else {
            String iccid = modem.getSimCCID();
            if (iccid.length() > 0) {
                apnResult = apnDetector.detectFromICCID(iccid.c_str());
            }
        }

        if (!apnResult.detected) {
            if (strlen(APN) > 0) {
                apnName = APN;
                apnUser = USER;
                apnPass = PASS;
                Logger::info("GSM", "Using configured APN: " + apnName);
            } else {
                Logger::info("GSM", "No APN detected yet — will try common APNs during GPRS attach");
            }
        } else {
            apnName = String(apnResult.apn);
            apnUser = String(apnResult.user);
            apnPass = String(apnResult.pass);
            Logger::info("GSM", "APN detected: " + apnName);
        }

        int csq = modem.getSignalQuality();
        Logger::info("GSM", "Signal CSQ: " + String(csq));

        networkWaitStart = millis();
        ready = true;
        updateImei();

        xSemaphoreGiveRecursive(_mutex);
        Logger::info("GSM", "Modem initialization SUCCESS");
        return true;
    }

    Logger::error("GSM", "Modem initialization FAILED");
    ready = false;
    xSemaphoreGiveRecursive(_mutex);
    return false;
}

void GSMManager::resetAttachState() {
    gprsAttachState = GprsAttachState::IDLE;
    gprsStateStartTime = 0;
    gprsLastTry = 0;
}

void GSMManager::loop() {
    if (!ready) return;

    unsigned long now = millis();

    if (now - lastCheck > 5000) {
        checkNetworkStatus();
        lastCheck = now;
    }

    if (networkRegistered && !gprsAttached) {
        attemptGprsAttach();
    }

    if (gprsAttached) {
        bool isGprs = false;

        if (_mutex && xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(100))) {
            isGprs = modem.isGprsConnected();
            xSemaphoreGiveRecursive(_mutex);
        }

        if (!isGprs) {
            if (gprsDisconnectDebounce < 6) {
                gprsDisconnectDebounce++;
            } else {
                static bool detachInProgress = false;
                static unsigned long detachStartTime = 0;

                if (!detachInProgress) {
                    Logger::warn("GSM", "GPRS lost — cleaning up and reattaching");

                    if (_mutex && xSemaphoreTakeRecursive(_mutex, portMAX_DELAY)) {
                        client.stop();
                        modem.gprsDisconnect();
                        xSemaphoreGiveRecursive(_mutex);
                    }

                    detachStartTime = millis();
                    detachInProgress = true;
                }

                if (detachInProgress && millis() - detachStartTime >= 1000) {
                    detachInProgress = false;
                    gprsAttached = false;
                    resetAttachState();
                    Logger::info("GSM", "Restarting GPRS attach...");
                    gprsDisconnectDebounce = 0;
                }
                esp_task_wdt_reset();
            }
        } else {
            gprsDisconnectDebounce = 0;
        }
    }
}

void GSMManager::resetModem() {
    resetCount++;
    Logger::warn("GSM", "--- RESETTING MODEM (Attempt " + String(resetCount) + ") ---");

    bool forceHardReset = (resetCount % 2 == 0);
    bool softResetSuccess = false;

    if (!forceHardReset) {
        if (_mutex && xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(5000))) {
            Logger::info("GSM", "Soft reset (AT)...");
            if (modem.restart()) {
                softResetSuccess = true;
                Logger::info("GSM", "Soft reset OK");
            }
            xSemaphoreGiveRecursive(_mutex);
        }
    }

    if (forceHardReset || !softResetSuccess) {
        powerOnModem();
    }

    if (_mutex && xSemaphoreTakeRecursive(_mutex, portMAX_DELAY)) {
        Logger::info("GSM", "Re-initializing after reset...");
        bool reInit = modem.init();
        if (!reInit) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            reInit = modem.init();
        }

        if (reInit) {
            modem.sendAT(GF("+CNMP=2")); modem.waitResponse();
            modem.sendAT(GF("+CMNB=1")); modem.waitResponse();
            modem.sendAT(GF("+CFUN=1")); modem.waitResponse(10000L);
            ready = true;
            updateImei();
            Logger::info("GSM", "Re-init success");
        } else {
            ready = false;
            Logger::error("GSM", "Re-init failed after reset");
        }
        xSemaphoreGiveRecursive(_mutex);
    }

    networkRegistered = false;
    gprsAttached = false;
    resetAttachState();
    networkWaitStart = millis();
}

void GSMManager::checkNetworkStatus() {
    if (!_mutex) return;

    int signal = 0;
    bool registered = false;

    if (xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(200))) {
        signal = modem.getSignalQuality();
        registered = modem.isNetworkConnected();
        xSemaphoreGiveRecursive(_mutex);
    } else {
        return;
    }

    if (registered && !networkRegistered) {
        Logger::info("GSM", "Network registered! CSQ=" + String(signal));
        networkRegistered = true;
        networkWaitStart = 0;
    } else if (!registered && networkRegistered) {
        Logger::warn("GSM", "Network registration lost");
        networkRegistered = false;
        gprsAttached = false;
        resetAttachState();
        networkWaitStart = millis();
    }

    if (!networkRegistered && networkWaitStart > 0) {
        if ((millis() - networkWaitStart) % 10000 < 100) {
            Logger::info("GSM", "Waiting for network... CSQ=" + String(signal));
        }

        if (millis() - networkWaitStart > NETWORK_REGISTER_TIMEOUT_MS) {
            Logger::error("GSM", "Network registration timeout — resetting modem");
            resetModem();
        }
    }
}

void GSMManager::attemptGprsAttach() {
    switch (gprsAttachState) {
        case GprsAttachState::IDLE:
            if (gprsAttached) {
                gprsAttachState = GprsAttachState::SUCCESS;
                break;
            }
            gprsAttachState = GprsAttachState::START;
            gprsStateStartTime = millis();
            Logger::info("GSM", "Starting GPRS attach...");
            break;

        case GprsAttachState::START:
            if (millis() - gprsLastTry < 1000) break;
            gprsLastTry = millis();

            Logger::info("GSM", "GPRS connect APN=" + apnName);

            {
                bool connected = false;
                if (_mutex && xSemaphoreTakeRecursive(_mutex, portMAX_DELAY)) {
                    connected = modem.gprsConnect(apnName.c_str(), apnUser.c_str(), apnPass.c_str());
                    xSemaphoreGiveRecursive(_mutex);
                }

                if (connected) {
                    gprsAttachState = GprsAttachState::SUCCESS;
                    gprsAttached = true;
                    String ip = sanitizeIp(getLocalIP());
                    Logger::info("GSM", "GPRS connected! IP=" + ip);
                } else {
                    gprsAttachState = GprsAttachState::WAITING;
                    gprsStateStartTime = millis();
                    Logger::warn("GSM", "GPRS attach failed, will retry");
                }
            }
            break;

        case GprsAttachState::WAITING:
            if (millis() - gprsStateStartTime > 3000) {
                gprsAttachState = GprsAttachState::START;
            }
            if (millis() - gprsStateStartTime > GPRS_ATTACH_TIMEOUT_MS) {
                Logger::error("GSM", "GPRS attach timeout");
                gprsAttachState = GprsAttachState::FAIL;
            }
            break;

        case GprsAttachState::SUCCESS:
            break;

        case GprsAttachState::FAIL:
            Logger::warn("GSM", "GPRS attach retry from FAIL state");
            gprsAttached = false;
            gprsAttachState = GprsAttachState::IDLE;
            break;
    }

    yield();
    esp_task_wdt_reset();
}

bool GSMManager::isNetworkReady() {
    return networkRegistered;
}

bool GSMManager::checkGprsReal() {
    bool connected = false;
    if (_mutex && xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(500))) {
        connected = modem.isGprsConnected();
        xSemaphoreGiveRecursive(_mutex);
    }
    if (!connected) gprsAttached = false;
    return connected;
}

void GSMManager::forceGprsReconnect() {
    Logger::warn("GSM", "Forcing GPRS reconnect...");
    gprsAttached = false;
    resetAttachState();

    if (_mutex && xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(1000))) {
        modem.gprsDisconnect();
        xSemaphoreGiveRecursive(_mutex);
    }
}

bool GSMManager::isGprsConnected() {
    return gprsAttached;
}

bool GSMManager::isGprsFullyReady() {
    if (!gprsAttached) return false;
    String ip = sanitizeIp(getLocalIP());
    return (ip.length() > 0 && ip != "0.0.0.0" && ip.indexOf('.') > 0);
}

bool GSMManager::tryGprsConnect(const char* apn, const char* user, const char* pass) {
    if (_mutex && xSemaphoreTakeRecursive(_mutex, portMAX_DELAY)) {
        bool res = modem.gprsConnect(apn, user ? user : "", pass ? pass : "");
        xSemaphoreGiveRecursive(_mutex);
        return res;
    }
    return false;
}

String GSMManager::getLocalIP() {
    if (!gprsAttached) return "";

    String ip = "";
    if (_mutex && xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(100))) {
        ip = modem.getLocalIP();
        xSemaphoreGiveRecursive(_mutex);
    }
    return ip;
}

TinyGsmClient& GSMManager::getClient() {
    return client;
}

int GSMManager::getSignalStrength() {
    int signal = 0;
    if (_mutex && xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(100))) {
        signal = modem.getSignalQuality();
        xSemaphoreGiveRecursive(_mutex);
    }
    return signal;
}

String GSMManager::getNetworkType() {
    return "LTE";
}

NetworkStatus GSMManager::getStatus() {
    NetworkStatus status;
    status.signalStrength = getSignalStrength();
    status.networkRegistered = networkRegistered;
    status.gprsConnected = gprsAttached;
    status.networkType = getNetworkType();
    return status;
}

time_t GSMManager::getNetworkTime() {
    if (!_mutex) return 0;

    String response;
    if (xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(1000))) {
        modem.sendAT(GF("+CCLK?"));
        if (modem.waitResponse(2000, response) != 1) {
            xSemaphoreGiveRecursive(_mutex);
            return 0;
        }
        xSemaphoreGiveRecursive(_mutex);
    } else {
        return 0;
    }

    int idx = response.indexOf(F("+CCLK:"));
    if (idx == -1) return 0;

    int quote1 = response.indexOf('"', idx);
    int quote2 = response.indexOf('"', quote1 + 1);
    if (quote1 == -1 || quote2 == -1) return 0;

    String timeStr = response.substring(quote1 + 1, quote2);

    int yy, MM, dd, hh, mm, ss;
    char tzSign = 0;
    int tzQuarterHours = 0;

    int count = sscanf(timeStr.c_str(), "%2d/%2d/%2d,%2d:%2d:%2d%c%2d",
                       &yy, &MM, &dd, &hh, &mm, &ss, &tzSign, &tzQuarterHours);

    if (count < 6) return 0;

    struct tm tm;
    tm.tm_year = (2000 + yy) - 1900;
    tm.tm_mon = MM - 1;
    tm.tm_mday = dd;
    tm.tm_hour = hh;
    tm.tm_min = mm;
    tm.tm_sec = ss;
    tm.tm_isdst = 0;

    time_t t = mktime(&tm);

    if (count >= 8 && (tzSign == '+' || tzSign == '-')) {
        int offsetSeconds = tzQuarterHours * 15 * 60;
        if (tzSign == '+') t -= offsetSeconds;
        else t += offsetSeconds;
    }

    return t;
}
