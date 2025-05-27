#pragma once

void debug_outc(char c)
{
    outb(0xe9, c);
}

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

char* LOG_LEVEL_STR[6] = 
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"     // Changed from CRITICAL because it overflows the 4 space tab of vscode
};

int _printf(void (*func)(char), void (*func_s)(char*), const char* format, va_list args);

int printf_log(const char* format, ...)
{
    void _putc(char c)
    {
        write(STDERR_FILENO, &c, 1);
    }
    void _puts(char* s)
    {
        write(STDERR_FILENO, s, strlen(s));
    }

    va_list args;
    va_start(args, format);
    int length = _printf(_putc, _puts, format, args);
    va_end(args);
    return length;
}

#ifndef LOG_LEVEL 
#define _LOG(level_text, ...)
#define LOG(level, ...)
#else
#define _LOG(level_text, ...)           { \
    if (time_initialized) \
        printf_log("%u-%u%u-%u%u \t %u%u:%u%u:%u%u,%u%u%u \t %s \t", \
        system_year, system_month / 10, system_month % 10, system_day / 10, system_day % 10, system_hours / 10, system_hours % 10, system_minutes / 10, system_minutes % 10, system_seconds / 10, system_seconds % 10, \
        system_thousands / 100, (system_thousands / 10) % 10, system_thousands % 10, level_text); \
        else printf_log("0000-00-00 \t 00:00:00,000 \t %s \t", level_text); \
        printf_log(__VA_ARGS__); printf_log("\n"); }
#define LOG(level, ...)                 do { if (LOG_LEVEL <= level) { _LOG(LOG_LEVEL_STR[level], __VA_ARGS__) } } while(0)
#endif