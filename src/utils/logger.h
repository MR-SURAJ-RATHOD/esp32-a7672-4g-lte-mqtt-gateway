#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "../include/gw_types.h"

class Logger {
public:
    static void begin();
    static void log(LogLevel level, const char* module, const char* message);
    static void log(LogLevel level, const char* module, const String& message);
    
    static void debug(const char* module, const char* message);
    static void info(const char* module, const char* message);
    static void warn(const char* module, const char* message);
    static void error(const char* module, const char* message);
    
    static void debug(const char* module, const String& message);
    static void info(const char* module, const String& message);
    static void warn(const char* module, const String& message);
    static void error(const char* module, const String& message);

private:
    static const char* levelToString(LogLevel level);
    static bool initialized;
};

#endif // LOGGER_H
