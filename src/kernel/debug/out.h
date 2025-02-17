#pragma once

void debug_outc(char c)
{
    outb(0xe9, c);
}

#define LOG_LEVEL_DEBUG     0
#define LOG_LEVEL_INFO      1
#define LOG_LEVEL_WARNING   2
#define LOG_LEVEL_ERROR     3
#define LOG_LEVEL_CRITICAL  4

#define DEBUG               LOG_LEVEL_DEBUG
#define INFO                LOG_LEVEL_INFO
#define WARNING             LOG_LEVEL_WARNING
#define ERROR               LOG_LEVEL_ERROR
#define CRITICAL            LOG_LEVEL_CRITICAL

#define LOG_LEVEL           DEBUG

char* LOG_LEVEL_STR[5] = 
{
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"     // Changed from CRITICAL because it overflows the 4 space tab of vscode
};

#define _LOG(level_text, ...)           if (time_initialized) kfprintf(klog, "%u-%u-%u \t %u:%u:%u,%u%u%u \t %s \t", system_year, system_month, system_day, system_hours, system_minutes, system_seconds, system_thousands / 100, (system_thousands / 10) % 10, system_thousands % 10, level_text); else kfprintf(klog, "0000-00-00 \t 00:00:00,000 \t %s \t", level_text); kfprintf(klog, __VA_ARGS__); kfprintf(klog, "\n");
#define LOG(level, ...)                 if (LOG_LEVEL <= level) { _LOG(LOG_LEVEL_STR[level], __VA_ARGS__) } 