#pragma once

#define PIT_CHANNEL_0_DATA  0x40
#define PIT_CHANNEL_1_DATA  0x41
#define PIT_CHANNEL_2_DATA  0x42
#define PIT_MODE_CMD        0x43

uint32_t global_timer = 0; // in milliseconds

void handle_irq_0();
void pit_channel_0_set_frequency(uint32_t frequency);
void ksleep(uint32_t milliseconds);