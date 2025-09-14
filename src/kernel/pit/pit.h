#pragma once

#include "../time/time.h"

#define PIT_CHANNEL_0_DATA  0x40
#define PIT_CHANNEL_1_DATA  0x41
#define PIT_CHANNEL_2_DATA  0x42
#define PIT_MODE_CMD        0x43

#define PIT_FREQUENCY       200 // 20
#define PIT_INCREMENT       (precise_time_ticks_per_second / PIT_FREQUENCY)

#define PIT_MODE_CHANNEL_0  0b00000000
#define PIT_MODE_CHANNEL_1  0b01000000
#define PIT_MODE_CHANNEL_2  0b10000000

#define PIT_MODE_CHANNEL(n) ((n & 0b11) << 6)

#define PIT_ACCESS_MODE_LATCH_COUNT 0b00000000
#define PIT_ACCESS_MODE_LOBYTE_ONLY 0b00010000
#define PIT_ACCESS_MODE_HIBYTE_ONLY 0b00100000
#define PIT_ACCESS_MODE_LOHIBYTE    0b00110000

#define PIT_OPERATING_MODE(n)        ((n & 0b111) << 1)

#define PIT_BINARY_MODE             0b00000000

volatile precise_time_t global_timer = 0;

void handle_irq_0(bool* ts);
void pit_channel_0_set_frequency(uint32_t frequency);
void ksleep(precise_time_t time);