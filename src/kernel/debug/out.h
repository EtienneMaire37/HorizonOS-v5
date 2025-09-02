#pragma once

#include "out_symbols.h"

void debug_outc(char c)
{
    outb(0xe9, c);
}

char* LOG_LEVEL_STR[6] = 
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"     // Changed from CRITICAL because it overflows the 4 space tab of vscode
};

bool first_log = true;

#ifndef LOG_LEVEL 
#define _LOG(level_text, ...)
#define LOG(level, ...)
#else
#define _LOG(level_text, ...)           { \
    if (!first_log) fprintf(stderr, "\n"); \
    first_log = false;  \
    if (time_initialized) \
        fprintf(stderr, "%u-%u%u-%u%u \t %u%u:%u%u:%u%u,%u%u%u \t %s \t", \
        system_year, system_month / 10, system_month % 10, system_day / 10, system_day % 10, system_hours / 10, system_hours % 10, system_minutes / 10, system_minutes % 10, system_seconds / 10, system_seconds % 10, \
        system_thousands / 100, (system_thousands / 10) % 10, system_thousands % 10, level_text); \
        else fprintf(stderr, "0000-00-00 \t 00:00:00,000 \t %s \t", level_text); \
        fprintf(stderr, __VA_ARGS__); }
#define LOG(level, ...)                 do { if (LOG_LEVEL <= level) { _LOG(LOG_LEVEL_STR[level], __VA_ARGS__) } } while(0)
#endif