/**
 * @file apn_detector.h
 * @brief Automatic APN Detection Module
 * 
 * Detects the correct APN based on SIM card IMSI/ICCID
 * Supports major carriers worldwide
 */

#ifndef APN_DETECTOR_H
#define APN_DETECTOR_H

#include <Arduino.h>
#include "gw_config.h"

/**
 * @brief APN configuration structure
 */
typedef struct {
    const char* mccMnc;         // Mobile Country Code + Mobile Network Code
    const char* carrier;        // Carrier name
    const char* apn;            // APN name
    const char* user;           // APN username (empty if not required)
    const char* pass;           // APN password (empty if not required)
} ApnConfig_t;

/**
 * @brief APN detection result
 */
typedef struct {
    bool detected;
    char apn[64];
    char user[32];
    char pass[32];
    char carrier[64];
    char mccMnc[8];
} ApnResult_t;

/**
 * @brief APN Detector Class
 * 
 * Automatically detects the correct APN based on SIM card information
 */
class ApnDetector {
public:
    /**
     * @brief Constructor
     */
    ApnDetector();
    
    /**
     * @brief Detect APN from IMSI
     * @param imsi IMSI string from SIM
     * @return APN detection result
     */
    ApnResult_t detectFromIMSI(const char* imsi);
    
    /**
     * @brief Detect APN from ICCID
     * @param iccid ICCID string from SIM
     * @return APN detection result
     */
    ApnResult_t detectFromICCID(const char* iccid);
    
    /**
     * @brief Detect APN from MCC+MNC
     * @param mccMnc MCC+MNC string (e.g., "40401" for Vodafone India)
     * @return APN detection result
     */
    ApnResult_t detectFromMccMnc(const char* mccMnc);
    
    /**
     * @brief Try common APNs until one works
     * @param testCallback Function to test if APN works
     * @return APN detection result
     */
    ApnResult_t detectByTrial(bool (*testCallback)(const char* apn, const char* user, const char* pass));
    
    /**
     * @brief Get carrier name for MCC+MNC
     * @param mccMnc MCC+MNC string
     * @return Carrier name or "Unknown"
     */
    const char* getCarrierName(const char* mccMnc);
    
    /**
     * @brief Add custom APN to database
     * @param mccMnc MCC+MNC code
     * @param carrier Carrier name
     * @param apn APN name
     * @param user Username
     * @param pass Password
     */
    void addCustomApn(const char* mccMnc, const char* carrier, 
                      const char* apn, const char* user, const char* pass);

private:
    // Custom APN storage
    static const int MAX_CUSTOM_APNS = 5;
    ApnConfig_t _customApns[MAX_CUSTOM_APNS];
    int _customApnCount;
    
    /**
     * @brief Search APN database
     * @param mccMnc MCC+MNC to search
     * @return Pointer to ApnConfig or nullptr
     */
    const ApnConfig_t* searchDatabase(const char* mccMnc);
    
    /**
     * @brief Extract MCC+MNC from IMSI
     * @param imsi IMSI string
     * @param mccMnc Output buffer (min 8 chars)
     */
    void extractMccMncFromIMSI(const char* imsi, char* mccMnc);
    
    /**
     * @brief Extract MCC from ICCID
     * @param iccid ICCID string
     * @param mcc Output buffer (min 4 chars)
     */
    void extractMccFromICCID(const char* iccid, char* mcc);
};

// Global APN detector instance
extern ApnDetector apnDetector;

// ============================================================================
// APN DATABASE
// ============================================================================

/**
 * @brief Global APN database
 * Sorted by MCC+MNC for binary search
 * 
 * MCC = Mobile Country Code (3 digits)
 * MNC = Mobile Network Code (2-3 digits)
 */
static const ApnConfig_t APN_DATABASE[] = {
    // ========== INDIA (MCC: 404, 405) ==========
    {"40401", "Vodafone India", "www", "", ""},
    {"40402", "Airtel India", "airtelgprs.com", "", ""},
    {"40403", "Airtel India", "airtelgprs.com", "", ""},
    {"40404", "IDEA India", "internet", "", ""},
    {"40405", "Vodafone India", "www", "", ""},
    {"40410", "Airtel India", "airtelgprs.com", "", ""},
    {"40411", "Vodafone India", "www", "", ""},
    {"40412", "IDEA India", "internet", "", ""},
    {"40413", "Vodafone India", "www", "", ""},
    {"40414", "IDEA India", "internet", "", ""},
    {"40415", "Vodafone India", "www", "", ""},
    {"40416", "Airtel India", "airtelgprs.com", "", ""},
    {"40420", "Vodafone India", "www", "", ""},
    {"40427", "Vodafone India", "www", "", ""},
    {"40430", "Vodafone India", "www", "", ""},
    {"40443", "Vodafone India", "www", "", ""},
    {"40445", "Airtel India", "airtelgprs.com", "", ""},
    {"40449", "Airtel India", "airtelgprs.com", "", ""},
    {"40470", "IDEA India", "internet", "", ""},
    {"40490", "Airtel India", "airtelgprs.com", "", ""},
    {"40492", "Airtel India", "airtelgprs.com", "", ""},
    {"40493", "Airtel India", "airtelgprs.com", "", ""},
    {"40494", "Airtel India", "airtelgprs.com", "", ""},
    {"40495", "Airtel India", "airtelgprs.com", "", ""},
    {"40496", "Airtel India", "airtelgprs.com", "", ""},
    {"40497", "Airtel India", "airtelgprs.com", "", ""},
    {"40498", "Airtel India", "airtelgprs.com", "", ""},
    {"405025", "TATA Docomo", "TATA.DOCOMO.INTERNET", "", ""},
    {"405799", "IDEA India", "internet", "", ""},
    {"405840", "Jio India", "jionet", "", ""},
    {"405854", "Jio India", "jionet", "", ""},
    {"405855", "Jio India", "jionet", "", ""},
    {"405856", "Jio India", "jionet", "", ""},
    {"405857", "Jio India", "jionet", "", ""},
    {"405858", "Jio India", "jionet", "", ""},
    {"405859", "Jio India", "jionet", "", ""},
    {"405860", "Jio India", "jionet", "", ""},
    {"405861", "Jio India", "jionet", "", ""},
    {"405862", "Jio India", "jionet", "", ""},
    {"405863", "Jio India", "jionet", "", ""},
    {"405864", "Jio India", "jionet", "", ""},
    {"405865", "Jio India", "jionet", "", ""},
    {"405866", "Jio India", "jionet", "", ""},
    {"405867", "Jio India", "jionet", "", ""},
    {"405868", "Jio India", "jionet", "", ""},
    {"405869", "Jio India", "jionet", "", ""},
    {"405870", "Jio India", "jionet", "", ""},
    {"405871", "Jio India", "jionet", "", ""},
    {"405872", "Jio India", "jionet", "", ""},
    {"405873", "Jio India", "jionet", "", ""},
    {"405874", "Jio India", "jionet", "", ""},
    
    // ========== USA (MCC: 310, 311, 312) ==========
    {"310012", "Verizon", "vzwinternet", "", ""},
    {"310026", "T-Mobile USA", "fast.t-mobile.com", "", ""},
    {"310030", "AT&T", "broadband", "", ""},
    {"310070", "AT&T", "broadband", "", ""},
    {"310090", "AT&T", "broadband", "", ""},
    {"310150", "AT&T", "broadband", "", ""},
    {"310170", "AT&T", "broadband", "", ""},
    {"310260", "T-Mobile USA", "fast.t-mobile.com", "", ""},
    {"310410", "AT&T", "broadband", "", ""},
    {"311480", "Verizon", "vzwinternet", "", ""},
    {"312530", "Sprint", "sprint", "", ""},
    
    // ========== UK (MCC: 234, 235) ==========
    {"23410", "O2 UK", "m-bb.o2.co.uk", "o2bb", "password"},
    {"23415", "Vodafone UK", "wap.vodafone.co.uk", "wap", "wap"},
    {"23420", "3 UK", "three.co.uk", "", ""},
    {"23430", "EE UK", "everywhere", "eesecure", "secure"},
    {"23433", "EE UK", "everywhere", "eesecure", "secure"},
    {"23450", "JT UK", "pepper", "", ""},
    {"23455", "Sure UK", "internet", "", ""},
    
    // ========== Germany (MCC: 262) ==========
    {"26201", "Telekom DE", "internet.t-mobile", "t-mobile", "tm"},
    {"26202", "Vodafone DE", "web.vodafone.de", "", ""},
    {"26203", "O2 DE", "internet", "", ""},
    {"26207", "O2 DE", "internet", "", ""},
    
    // ========== France (MCC: 208) ==========
    {"20801", "Orange FR", "orange.fr", "orange", "orange"},
    {"20810", "SFR FR", "sl2sfr", "", ""},
    {"20815", "Free FR", "free", "", ""},
    {"20820", "Bouygues FR", "mmsbouygtel.com", "", ""},
    
    // ========== Australia (MCC: 505) ==========
    {"50501", "Telstra", "telstra.internet", "", ""},
    {"50502", "Optus", "yesinternet", "", ""},
    {"50503", "Vodafone AU", "live.vodafone.com", "", ""},
    {"50506", "3 AU", "3netaccess", "", ""},
    
    // ========== China (MCC: 460) ==========
    {"46000", "China Mobile", "cmnet", "", ""},
    {"46001", "China Unicom", "3gnet", "", ""},
    {"46002", "China Mobile", "cmnet", "", ""},
    {"46003", "China Telecom", "ctnet", "", ""},
    {"46011", "China Telecom", "ctnet", "", ""},
    
    // ========== UAE (MCC: 424) ==========
    {"42402", "Etisalat", "mnet", "", ""},
    {"42403", "Du", "du", "", ""},
    
    // ========== Saudi Arabia (MCC: 420) ==========
    {"42001", "STC", "jawalnet.com.sa", "", ""},
    {"42003", "Mobily", "web1", "", ""},
    {"42004", "Zain SA", "zain", "", ""},
    
    // ========== Singapore (MCC: 525) ==========
    {"52501", "SingTel", "e-ideas", "", ""},
    {"52502", "SingTel", "e-ideas", "", ""},
    {"52503", "M1", "sunsurf", "", ""},
    {"52505", "StarHub", "shwap", "", ""},
    
    // ========== Generic/Fallback ==========
    {"00000", "Generic", "internet", "", ""},
};

static const int APN_DATABASE_SIZE = sizeof(APN_DATABASE) / sizeof(APN_DATABASE[0]);

// Common APNs to try if detection fails
static const char* COMMON_APNS[] = {
    "internet",
    "web",
    "data",
    "wap",
    "broadband",
    "mobile",
    "default",
    nullptr  // Terminator
};

#endif // APN_DETECTOR_H
