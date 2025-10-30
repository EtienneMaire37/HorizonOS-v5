#pragma once

#include "time.h"
#include "gdn.h"

time_t ktime(time_t* t)
{
    time_t now = time_to_unix(system_year, system_month, system_day, system_hours, system_minutes, system_seconds);
    if (t) *t = now;
    return now;
}