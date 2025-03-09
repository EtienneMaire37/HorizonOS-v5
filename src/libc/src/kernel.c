#pragma once

int fputc(int c, FILE* stream)
{
    switch((unsigned int)stream)
    {
    case (unsigned int)klog:
        debug_outc((char)c);
        break;
    case (unsigned int)stderr:
    case (unsigned int)stdout:
        tty_outc((char)c);
        tty_update_cursor();
        break;

    default:
        LOG(ERROR, "Invalid output stream");
        errno = EBADF;
        return EOF;
    }

    return c;
}

void exit(int r)
{
    LOG(CRITICAL, "Kernel aborted");
    fprintf(stderr, "\nKernel aborted.");
    halt();
}

time_t time(time_t* t)
{
    return ktime(t);
}

pid_t getpid()
{
    return (pid_t)-1;
}

pid_t fork()
{
    return (pid_t)-1;
}