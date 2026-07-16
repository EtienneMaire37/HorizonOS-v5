#include <unistd.h>
#include "../terminal/textio.h"
#include "../debug/out.h"

ssize_t write(int fd, const void* buf, size_t n)
{
    switch(fd)
    {
    case STDOUT_FILENO:
        for (size_t i = 0; i < n; i++)
            tty_outc_ex(((char*)buf)[i], 0);
        __tty_refresh_screen();
        return n;
    case STDERR_FILENO:
        for (size_t i = 0; i < n; i++)
            debug_outc(((char*)buf)[i]);
        return n;
    default:
        return 0;
    }
}
