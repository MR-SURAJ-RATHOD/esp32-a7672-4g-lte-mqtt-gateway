#include "logger.h"
#include "gw_config.h"

bool Logger::initialized = false;

void Logger::begin() {
    if (!initialized) {
        SerialMon.begin(BAUD_RATE);
        delay(100);
        initialized = true;
        info("Logger", "Logger initialized");
    }
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void Logger::log(LogLevel level, const char* module, const char* message) {
    if (!initialized) return;
    
    unsigned long now = millis();
    SerialMon.printf("[%8lu] [%s] [%s] %s\n", now, levelToString(level), module, message);
}

void Logger::log(LogLevel level, const char* module, const String& message) {
    log(level, module, message.c_str());
}

void Logger::debug(const char* module, const char* message) {
    log(LogLevel::DEBUG, module, message);
}

void Logger::info(const char* module, const char* message) {
    log(LogLevel::INFO, module, message);
}

void Logger::warn(const char* module, const char* message) {
    log(LogLevel::WARN, module, message);
}

void Logger::error(const char* module, const char* message) {
    log(LogLevel::ERROR, module, message);
}

void Logger::debug(const char* module, const String& message) {
    debug(module, message.c_str());
}

void Logger::info(const char* module, const String& message) {
    info(module, message.c_str());
}

void Logger::warn(const char* module, const String& message) {
    warn(module, message.c_str());
}

void Logger::error(const char* module, const String& message) {
    error(module, message.c_str());
}
