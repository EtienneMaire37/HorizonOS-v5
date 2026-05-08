#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

time_t ktime(time_t* t);
bool is_leap_year(int64_t year);
uint8_t get_num_days_in_month(int64_t month, int64_t year);
void resolve_time();
