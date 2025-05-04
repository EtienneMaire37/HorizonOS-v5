#include "../include/math.h"
#include "math_float_util.h"
// #include "../src/math.c"

int putchar(int c)
{
    write(STDOUT_FILENO, &c, 1);
    return c;
}

int puts(const char* s)
{
    write(STDOUT_FILENO, s, strlen(s));
    return 1;
}

#define DOUBLE_PRINT_MAX_DIGITS         34      // 18
#define LONG_DOUBLE_PRINT_MAX_DIGITS    308     // 18

#define DOUBLE_PRINT_LIMIT      1e-10
#define LONG_DOUBLE_PRINT_LIMIT 1e-15

int _printf(void (*func)(char), void (*func_s)(char*), const char* format, va_list args)
{
    char hex[16] = "0123456789abcdef";
    char HEX[16] = "0123456789ABCDEF";

    uint32_t length = 0;

    void printf_d(int64_t val)
    {
        if(val < 0)
        {
            func('-');
            length++;
            val = -val;
        }
        if(val == 0)
        {
            func('0');
            length++;
            return;
        }
        int64_t div = 1000000000000000000ULL;
        bool first0 = true;
        while(div > 0)
        {
            uint8_t digit = (val / div) % 10;
            if(digit || div == 1)
                first0 = false;
            if(!first0)
            {
                func('0' + digit);
                length++;
            }
            div /= 10;
        }
    }
    void printf_u(uint64_t val)
    {
        if(val == 0)
        {
            func('0');
            length++;
            return;
        }
        uint64_t div = 10000000000000000000ULL;
        bool first0 = true;
        while(div > 0)
        {
            uint8_t digit = (val / div) % 10;
            if(digit || div == 1)
                first0 = false;
            if(!first0)
            {
                func('0' + digit);
                length++;
            }
            div /= 10;
        }
    }
    void printf_x(uint64_t val, uint8_t padding)
    {
        bool first0 = true;
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t digit = (val >> ((uint16_t)(15 - i) * 4)) & 0xf;
            first0 &= digit == 0;
            first0 &= i < 16 - padding;
            if(!first0)
            {
                func(hex[digit]);
                length++;
            }
        }
    }
    void printf_fld(long double val)
    {
        if(val < 0)
        {
            func('-');
            length++;
            val = -val;
        }
        if(val == 0)
        {
            func('0');
            length++;
            return;
        }
        bool first0 = true;
        uint16_t digits = 0;
        long double div = 1;
        while (val / div >= 1) div *= 10;
        div /= 10;
        while(div > 1)
        {
            uint8_t digit = (uint8_t)fmodl(floorl(val / div), 10.L);
            if(digit || div == 1)
                first0 = false;
            if(!first0)
            {
                func('0' + digit);
                length++;
            }
            div /= 10;
        }
    }
    void printf_fd(double val)
    {
        if(val < 0)
        {
            func('-');
            length++;
            val = -val;
        }
        if(val == 0)
        {
            func('0');
            length++;
            return;
        }
        bool first0 = true;
        uint16_t digits = 0;
        double div = 1;
        while (val / div >= 1) div *= 10;
        div /= 10;
        while(div > 1)
        {
            uint8_t digit = (uint8_t)fmod(floor(val / div), 10.);
            if(digit || div == 1)
                first0 = false;
            if(!first0)
            {
                func('0' + digit);
                length++;
            }
            div /= 10;
        }
    }
    void printf_X(uint64_t val, uint8_t padding)
    {
        bool first0 = true;
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t digit = (val >> ((uint16_t)(15 - i) * 4)) & 0xf;
            first0 &= digit == 0;
            first0 &= i < 16 - padding;
            if(!first0)
            {
                func(HEX[digit]);
                length++;
            }
        }
    }
    void printf_lf(long double val)
    {
#include "math_fmod.c"
        if (val < 0)
        {
            func('-');
            length++;
            val *= -1;
        }
        
        switch(fpclassify(val))
        {
        case FP_INFINITE:
        {
            func_s("inf");
            length += 3;
            return;
        }
        case FP_NAN:
        {
            func_s("nan");
            length += 3;
            return;
        }
        }

        long double k = floorl(val);
        long double p = val - k;
        printf_fld(k);
        if (p < LONG_DOUBLE_PRINT_LIMIT)
            return;
        func('.');
        length++;
        uint64_t mul = 10;
        uint64_t max_mul = 10;
        long double _p = p;
        uint16_t digits = 0;
        while (_p > LONG_DOUBLE_PRINT_LIMIT && digits < LONG_DOUBLE_PRINT_MAX_DIGITS)
        {
            uint8_t digit = (((uint8_t)floorl(_p * mul)) % 10);
            func('0' + digit);
            length++;
            _p -= digit / (long double)mul;
            mul *= 10;
            digits++;
        }
    }
    void printf_f(double val)
    {
#include "math_fmod.c"
        if (val < 0)
        {
            func('-');
            length++;
            val *= -1;
        }

        switch(fpclassify(val))
        {
        case FP_INFINITE:
        {
            func_s("inf");
            length += 3;
            return;
        }
        case FP_NAN:
        {
            func_s("nan");
            length += 3;
            return;
        }
        }

        double k = floor(val);
        double p = val - k;
        printf_fd(k);
        if (p < DOUBLE_PRINT_LIMIT)
            return;
        func('.');
        length++;
        uint64_t mul = 10;
        double _p = p;
        uint8_t digits = 0;
        while (_p > DOUBLE_PRINT_LIMIT && digits < DOUBLE_PRINT_MAX_DIGITS)
        {
            uint8_t digit = (((uint8_t)floor(_p * mul)) % 10);
            func('0' + digit);
            length++;
            _p -= digit / (double)mul;
            mul *= 10;
            digits++;
        }
    }

    bool next_arg_64 = false, na64_set = false;
    bool next_formatted = false;
    while (*format)
    {
        if (*format == '%')
        {
            if (next_formatted)
            {
                func('%');
                next_formatted = false;
            }
            else
                next_formatted = true;
            format++;
            continue;
        }
        if (next_formatted)
        {
            switch (*format)
            {
            case 'd':
                if (next_arg_64)
                    printf_d(va_arg(args, int64_t));
                else
                    printf_d(va_arg(args, int32_t));
                break;
            case 'u':
                if (next_arg_64)
                    printf_u(va_arg(args, uint64_t));
                else
                    printf_u(va_arg(args, uint32_t));
                break;
            case 'x':
                if (next_arg_64)
                    printf_x(va_arg(args, uint64_t), 1);
                else
                    printf_x(va_arg(args, uint32_t), 1);
                break;
            case 'X':
                if (next_arg_64)
                    printf_X(va_arg(args, uint64_t), 1);
                else
                    printf_X(va_arg(args, uint32_t), 1);
                break;
            case 'f':
                if (next_arg_64)
                    printf_lf(va_arg(args, long double));
                else
                    printf_f(va_arg(args, double));
                break;
            case 'c':
                func((char)va_arg(args, uint32_t));
                length++;
                break;
            case 'l':
                na64_set = true;
                break;
            case 's':
            {
                char* s = va_arg(args, char*);
                while(*s)
                {
                    func(*s++);
                    length++;
                }
                break;
            }
                
            default:
                break;
            }
            if (na64_set)
            {
                na64_set = false;
                next_arg_64 = true;
                next_formatted = true;
            }
            else
            {
                next_arg_64 = false;
                next_formatted = false;
            }
            format++;
            continue;
        }
        else
        {
            func(*format);
            length++;
        }
        format++;
    }

    return length;
}

int printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int length = vprintf(format, args);
    va_end(args);
    return length;
}

int sprintf(char* buffer, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int length = vsprintf(buffer, format, args);
    va_end(args);
    return length;
}

int snprintf(char* buffer, size_t bufsz, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int length = vsnprintf(buffer, bufsz, format, args);
    va_end(args);
    return length;
}

int vsprintf(char* buffer, const char* format, va_list args)
{
    uint32_t index = 0;
    void _putc(char c)
    {
        buffer[index++] = c;
    }
    void _puts(char* s)
    {
        while (*s)
        {
            _putc(*s);
            s++;
        }
    }

    int length = _printf(_putc, _puts, format, args);
    buffer[index] = 0;
    return length;
}

int vsnprintf(char* buffer, size_t bufsz, const char* format, va_list args)
{
    if (bufsz == 0) return 0;

    uint32_t index = 0;
    void _putc(char c)
    {
        if (index < bufsz - 1)
            buffer[index++] = c;
    }
    void _puts(char* s)
    {
        while (*s)
        {
            _putc(*s);
            s++;
        }
    }

    int length = _printf(_putc, _puts, format, args);
    // if (bufsz != 0)
    buffer[index] = 0;
    return length;
}

int vprintf(const char* format, va_list args)
{
    void _putc(char c)
    {
        putchar(c);
    }
    void _puts(char* s)
    {
        puts(s);
    }

    return _printf(_putc, _puts, format, args);
}

int dprintf(int fd, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int length = vdprintf(fd, format, args);
    va_end(args);
    return length;
}

int vdprintf(int fd, const char *format, va_list args)
{
    void _putc(char c)
    {
        write(fd, &c, 1);
    }
    void _puts(char* s)
    {
        write(fd, s, strlen(s));
    }

    return _printf(_putc, _puts, format, args);
}

int getchar()
{
    static unsigned char buffer[BUFSIZ];
    static size_t start = 0, end = 0;
    ssize_t bytes_read;

    if (start >= end)
    {
        bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (bytes_read <= 0) 
            return EOF; 
        start = 0;
        end = (size_t)bytes_read;
    }

    return (int)buffer[start++];
}