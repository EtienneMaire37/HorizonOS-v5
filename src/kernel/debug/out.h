#pragma once

#define LOG_LEVEL_TRACE     0
#define LOG_LEVEL_DEBUG     1
#define LOG_LEVEL_INFO      2
#define LOG_LEVEL_WARNING   3
#define LOG_LEVEL_ERROR     4
#define LOG_LEVEL_CRITICAL  5

#define TRACE               LOG_LEVEL_TRACE
#define DEBUG               LOG_LEVEL_DEBUG
#define INFO                LOG_LEVEL_INFO
#define WARNING             LOG_LEVEL_WARNING
#define ERROR               LOG_LEVEL_ERROR
#define CRITICAL            LOG_LEVEL_CRITICAL

void debug_outc(char c)
{
    outb(0xe9, c);
}

// char* LOG_LEVEL_STR[6] = 
// {
//     "TRACE",
//     "DEBUG",
//     "INFO",
//     "WARN",
//     "ERROR",
//     "FATAL"     // Changed from CRITICAL because it overflows the 4 space tab of vscode
// };

char* LOG_LEVEL_STR[6] = 
{
    "[Trace]",
    "[Debug]",
    "[Info]",
    "[Warn]",
    "[Error]",
    "[Fatal]"
};

bool first_log = true;

// #define LOG_FMT         "%u-%u%u-%u%u \t %u%u:%u%u:%u%u,%u%u%u \t %s\t"
// #define LOG_FMT_NOTIME  "0000-00-00 \t 00:00:00,000 \t %s \t"

#define LOG_FMT         "%u-%u%u-%u%u %u%u:%u%u:%u%u.%u%u%u - %s \t"
#define LOG_FMT_NOTIME  "0000-00-00 00:00:00.000 - %s \t"

#ifndef LOG_LEVEL 
#define _LOG(level_text, ...)
#define LOG(level, ...)
#else
#define _LOG(level_text, ...) { \
    if (!first_log) fputc('\n', stderr); \
    first_log = false;  \
    if (time_initialized) \
        fprintf(stderr, LOG_FMT, \
        system_year, system_month / 10, system_month % 10, system_day / 10, system_day % 10, system_hours / 10, system_hours % 10, system_minutes / 10, system_minutes % 10, system_seconds / 10, system_seconds % 10, \
        system_thousands / 100, (system_thousands / 10) % 10, system_thousands % 10, level_text); \
    else fprintf(stderr, LOG_FMT_NOTIME, level_text); \
    fprintf(stderr, __VA_ARGS__); }
#define LOG(level, ...)                 do { if (LOG_LEVEL <= level && LOG_LEVEL >= 0 && LOG_LEVEL < sizeof(LOG_LEVEL_STR) / sizeof(char*)) { _LOG(LOG_LEVEL_STR[level], __VA_ARGS__) } } while(0)
#define CONTINUE_LOG(level, ...)    do { if (LOG_LEVEL <= level && LOG_LEVEL >= 0 && LOG_LEVEL < sizeof(LOG_LEVEL_STR) / sizeof(char*)) fprintf(stderr, __VA_ARGS__); } while (0)
#endif