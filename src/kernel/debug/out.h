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

// #define ANSI_COLORS

char* LOG_LEVEL_STR[] = 
{
    "[Trace]",
    "[Debug]",
    "[Info]",
    "[Warn]",
    "[Error]",
    "[Fatal]"
};

#ifndef ANSI_COLORS
char* LOG_LEVEL_COLOR[] = 
{
    "",
    "",
    "",
    "",
    "",
    ""
};
#else
char* LOG_LEVEL_COLOR[] = 
{
    "\x1b[38;2;130;130;130m\x1b[48;2;30;30;30m",   // Trace: gray on dark gray
    "\x1b[38;2;100;150;255m\x1b[48;2;20;30;60m",   // Debug: soft blue on navy
    "\x1b[38;2;120;200;120m\x1b[48;2;20;40;20m",   // Info: green on dark green
    "\x1b[38;2;255;200;100m\x1b[48;2;60;40;10m",   // Warn: amber on dark brown
    "\x1b[38;2;255;100;100m\x1b[48;2;50;10;10m",   // Error: red on deep red
    "\x1b[38;2;255;255;255m\x1b[48;2;150;0;0m"     // Fatal: white on dark red
};
#endif

bool first_log = true;

#ifndef ANSI_COLORS
#define LOG_FMT         "\
%u-%u%u-%u%u \
%u%u:%u%u:%u%u.%u%u%u\
 - %s%s \t"

#define LOG_FMT_NOTIME  "\
0000-00-00 00:00:00.000\
 - %s%s \t"
#else
#define LOG_FMT         "\
\x1b[38;2;200;200;200m\x1b[48;2;40;40;40m%u-%u%u-%u%u \x1b[0m\
\x1b[38;2;150;200;255m\x1b[48;2;40;40;40m%u%u:%u%u:%u%u.%u%u%u\x1b[0m\
 - %s%s\x1b[0m \t"

#define LOG_FMT_NOTIME  "\
\x1b[38;2;120;120;120m\x1b[48;2;30;30;30m0000-00-00 00:00:00.000\x1b[0m\
 - %s%s\x1b[0m \t"
#endif

// void LOG_SET_ANSI_TEXT_COLOR(uint8_t r, uint8_t g, uint8_t b)
// {
//     fprintf(stderr, "\x1b[38;2;%u;%u;%um", r, g, b);
// } 

// void LOG_SET_ANSI_BACKGROUND_COLOR(uint8_t r, uint8_t g, uint8_t b)
// {
//     fprintf(stderr, "\x1b[48;2;%u;%u;%um", r, g, b);
// } 

// void LOG_ANSI_RESET_STYLING()
// {
//     fputs("\x1b[0m", stderr);
// }

#ifndef LOG_LEVEL
#define LOG_LEVEL INFO
#endif

#ifndef LOG_LEVEL 
#define LOG(level, ...)
#define CONTINUE_LOG(level, ...)
#else
#define _LOG(level, ...) { \
    if (!first_log) fputc('\n', stderr); \
    first_log = false;  \
    if (time_initialized) \
        fprintf(stderr, LOG_FMT, \
        system_year, system_month / 10, system_month % 10, system_day / 10, system_day % 10, system_hours / 10, system_hours % 10, system_minutes / 10, system_minutes % 10, system_seconds / 10, system_seconds % 10, \
        system_thousands / 100, (system_thousands / 10) % 10, system_thousands % 10, LOG_LEVEL_COLOR[level], LOG_LEVEL_STR[level]); \
    else fprintf(stderr, LOG_FMT_NOTIME, LOG_LEVEL_COLOR[level], LOG_LEVEL_STR[level]); \
    fprintf(stderr, __VA_ARGS__); }
#define LOG(level, ...)                 do { if (LOG_LEVEL <= level && LOG_LEVEL >= 0 && LOG_LEVEL < sizeof(LOG_LEVEL_STR) / sizeof(char*)) { _LOG(level, __VA_ARGS__) } } while(0)
#define CONTINUE_LOG(level, ...)    do { if (LOG_LEVEL <= level && LOG_LEVEL >= 0 && LOG_LEVEL < sizeof(LOG_LEVEL_STR) / sizeof(char*)) fprintf(stderr, __VA_ARGS__); } while (0)
#endif