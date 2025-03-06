// time_t time(time_t* t)
// {
//     // time_t now = time_to_unix(system_year, system_month, system_day, system_hours, system_minutes, system_seconds);
//     time_t now = 0;
//     asm("int 0xff" : "=a" (now)
//         : "a" (2));
//     if (t) *t = now;
//     return now;
// }
