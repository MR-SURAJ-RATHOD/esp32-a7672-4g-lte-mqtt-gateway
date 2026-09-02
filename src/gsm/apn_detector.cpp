/**
 * @file apn_detector.cpp
 * @brief Automatic APN Detection Implementation
 */

#include "apn_detector.h"
#include <string.h>
#include "../utils/logger.h"


// Global instance
ApnDetector apnDetector;

ApnDetector::ApnDetector() : _customApnCount(0) {
    memset(_customApns, 0, sizeof(_customApns));
}

ApnResult_t ApnDetector::detectFromIMSI(const char* imsi) {
    ApnResult_t result;
    memset(&result, 0, sizeof(ApnResult_t));
    
    if (imsi == nullptr || strlen(imsi) < 5) {
        Logger::info("APN", "Invalid IMSI");
        return result;
    }
    
    // Extract MCC+MNC from IMSI (first 5-6 digits)
    char mccMnc[8];
    extractMccMncFromIMSI(imsi, mccMnc);
    
    Logger::info("APN", String("Detected MCC+MNC from IMSI: ") + mccMnc);

    return detectFromMccMnc(mccMnc);
}

ApnResult_t ApnDetector::detectFromICCID(const char* iccid) {
    ApnResult_t result;
    memset(&result, 0, sizeof(ApnResult_t));
    
    if (iccid == nullptr || strlen(iccid) < 7) {
        Logger::error("APN", "Invalid ICCID");
        return result;
    }
    
    // ICCID format: 89 (industry) + CC (country) + issuer + account
    // Country code is at positions 2-4
    char mcc[4];
    extractMccFromICCID(iccid, mcc);
    Logger::info("APN", String("Detected MCC from ICCId: ") + mcc);

    // Try to find by MCC only (less precise)
    // Search for any matching MCC in database
    for (int i = 0; i < APN_DATABASE_SIZE; i++) {
        if (strncmp(APN_DATABASE[i].mccMnc, mcc, strlen(mcc)) == 0) {
            result.detected = true;
            strncpy(result.mccMnc, APN_DATABASE[i].mccMnc, sizeof(result.mccMnc) - 1);
            strncpy(result.carrier, APN_DATABASE[i].carrier, sizeof(result.carrier) - 1);
            strncpy(result.apn, APN_DATABASE[i].apn, sizeof(result.apn) - 1);
            strncpy(result.user, APN_DATABASE[i].user, sizeof(result.user) - 1);
            strncpy(result.pass, APN_DATABASE[i].pass, sizeof(result.pass) - 1);
            Logger::info("APN", String("Found APN for MCC: ") + mcc + " , " + result.apn + " , " + result.carrier);
            return result;
        }
    }
    
    return result;
}

ApnResult_t ApnDetector::detectFromMccMnc(const char* mccMnc) {
    ApnResult_t result;
    memset(&result, 0, sizeof(ApnResult_t));
    
    if (mccMnc == nullptr || strlen(mccMnc) < 5) {
        Logger::error("APN", "Invalid MCC+MNC");
        return result;
    }
    
    strncpy(result.mccMnc, mccMnc, sizeof(result.mccMnc) - 1);
    
    // First check custom APNs
    for (int i = 0; i < _customApnCount; i++) {
        if (strcmp(_customApns[i].mccMnc, mccMnc) == 0) {
            result.detected = true;
            strncpy(result.carrier, _customApns[i].carrier, sizeof(result.carrier) - 1);
            strncpy(result.apn, _customApns[i].apn, sizeof(result.apn) - 1);
            strncpy(result.user, _customApns[i].user, sizeof(result.user) - 1);
            strncpy(result.pass, _customApns[i].pass, sizeof(result.pass) - 1);
            Logger::info("APN", String("Fount custom APN: ") + result.apn + " , " + result.carrier);
            return result;
        }
    }
    
    // Search database - try exact match first
    const ApnConfig_t* config = searchDatabase(mccMnc);
    
    if (config != nullptr) {
        result.detected = true;
        strncpy(result.carrier, config->carrier, sizeof(result.carrier) - 1);
        strncpy(result.apn, config->apn, sizeof(result.apn) - 1);
        strncpy(result.user, config->user, sizeof(result.user) - 1);
        strncpy(result.pass, config->pass, sizeof(result.pass) - 1);
        Logger::info("APN", String("Found APN: ") + result.apn + " (" + result.carrier + ")");
        return result;
    }
    
    // Try with 5-digit MCC+MNC if 6 digits didn't match
    if (strlen(mccMnc) == 6) {
        char shortMccMnc[6];
        strncpy(shortMccMnc, mccMnc, 5);
        shortMccMnc[5] = '\0';
        
        config = searchDatabase(shortMccMnc);
        if (config != nullptr) {
            result.detected = true;
            strncpy(result.carrier, config->carrier, sizeof(result.carrier) - 1);
            strncpy(result.apn, config->apn, sizeof(result.apn) - 1);
            strncpy(result.user, config->user, sizeof(result.user) - 1);
            strncpy(result.pass, config->pass, sizeof(result.pass) - 1);
            
            Logger::info("APN", String("Found APN (5 Digits): ") + result.apn + " (" + result.carrier + ")");
            return result;
        }
    }
    
    Logger::info("APN", String(" No APN found MCC+MNC: ") + mccMnc);
    return result;
}

ApnResult_t ApnDetector::detectByTrial(bool (*testCallback)(const char* apn, const char* user, const char* pass)) {
    ApnResult_t result;
    memset(&result, 0, sizeof(ApnResult_t));
    
    if (testCallback == nullptr) {
        return result;
    }
    
    Logger::info("APN", " Trying common APNs... ");
    
    // Try common APNs
    for (int i = 0; COMMON_APNS[i] != nullptr; i++) {
        
        Logger::info("APN", String(" Trying: ") + COMMON_APNS[i]);
        
        if (testCallback(COMMON_APNS[i], "", "")) {
            result.detected = true;
            strncpy(result.apn, COMMON_APNS[i], sizeof(result.apn) - 1);
            strcpy(result.carrier, "Unknown");
            
            Logger::info("APN", String(" Success with APN: ") + result.apn);

            return result;
        }
        
        delay(1000);  // Brief delay between attempts
    }
    Logger::error("APN", " No working APN found");
    return result;
}

const char* ApnDetector::getCarrierName(const char* mccMnc) {
    if (mccMnc == nullptr) {
        return "Unknown";
    }
    
    const ApnConfig_t* config = searchDatabase(mccMnc);
    if (config != nullptr) {
        return config->carrier;
    }
    
    return "Unknown";
}

void ApnDetector::addCustomApn(const char* mccMnc, const char* carrier, 
                               const char* apn, const char* user, const char* pass) {
    if (_customApnCount >= MAX_CUSTOM_APNS) {
        
        Logger::error("APN", " Custom APN storage full");

        return;
    }
    
    _customApns[_customApnCount].mccMnc = mccMnc;
    _customApns[_customApnCount].carrier = carrier;
    _customApns[_customApnCount].apn = apn;
    _customApns[_customApnCount].user = user;
    _customApns[_customApnCount].pass = pass;
    _customApnCount++;
                
    Logger::info("APN", String("  Added custom APN: ") + apn + " for " + mccMnc);

}

const ApnConfig_t* ApnDetector::searchDatabase(const char* mccMnc) {
    // Linear search (could be optimized to binary search since database is sorted)
    for (int i = 0; i < APN_DATABASE_SIZE; i++) {
        if (strcmp(APN_DATABASE[i].mccMnc, mccMnc) == 0) {
            return &APN_DATABASE[i];
        }
    }
    return nullptr;
}

void ApnDetector::extractMccMncFromIMSI(const char* imsi, char* mccMnc) {
    // IMSI format: MCC (3 digits) + MNC (2-3 digits) + MSIN
    // Most common is 5 digits (3+2), some use 6 (3+3)
    
    // Default to 5 digits
    strncpy(mccMnc, imsi, 5);
    mccMnc[5] = '\0';
    
    // Check if we should use 6 digits
    // Countries with 3-digit MNC: USA (310-316), Canada (302), etc.
    if (strncmp(imsi, "310", 3) == 0 ||
        strncmp(imsi, "311", 3) == 0 ||
        strncmp(imsi, "312", 3) == 0 ||
        strncmp(imsi, "313", 3) == 0 ||
        strncmp(imsi, "314", 3) == 0 ||
        strncmp(imsi, "315", 3) == 0 ||
        strncmp(imsi, "316", 3) == 0 ||
        strncmp(imsi, "302", 3) == 0 ||
        strncmp(imsi, "405", 3) == 0) {  // India Jio uses 6 digits
        strncpy(mccMnc, imsi, 6);
        mccMnc[6] = '\0';
    }
}

void ApnDetector::extractMccFromICCID(const char* iccid, char* mcc) {
    // ICCID format: 89 (MII) + CC (1-3 digits) + issuer + account
    // Position 2-4 typically contains country indicator
    
    // Skip "89" prefix
    if (iccid[0] == '8' && iccid[1] == '9') {
        strncpy(mcc, iccid + 2, 3);
        mcc[3] = '\0';
    } else {
        strncpy(mcc, iccid, 3);
        mcc[3] = '\0';
    }
}
