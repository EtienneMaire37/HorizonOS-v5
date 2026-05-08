#pragma once

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#define GDN_EPOCH 719162

uint32_t year_to_gdn(uint16_t year, bool* leap);
uint32_t time_to_gdn(uint16_t year, uint8_t month, uint8_t day);
time_t time_to_unix(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
