#pragma once

#include "apic.h"

void apic_timer_and_tsc_init();
void apic_timer_add_new_deadline(uint64_t tsc_deadline);
void apic_timer_handle_irq();
