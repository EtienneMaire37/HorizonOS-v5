// int fputc(int c, FILE* stream)
// {
//     asm("int 0xff" : 
//         : "a" (1), "b" ((char)c), "c" (stream));
//     return c;
// }

#include "../include/math.h"
// #include "../src/math.c"

int putchar(int c)
{
    return fputc(c, stdout);
}

int fputs(const char* s, FILE* stream)
{
    while(*s)
    {
        fputc(*s, stream);
        s++;
    }
    return 0;
}

int puts(const char* s)
{
    return fputs(s, stdout);
}

FILE* fopen(const char* path, const char* mode)
{
    errno = EACCES;
    return NULL;
}

int fclose(FILE* stream)
{
    errno = EBADF;
    return EOF;
}

#define FLOAT_PRINT_MAX_DIGITS  18 // 34
#define DOUBLE_PRINT_MAX_DIGITS 18 // 308

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
        int64_t div = 1000000000000000000;
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
        uint64_t div = 10000000000000000000;
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
        if(val == 0)
        {
            for(uint8_t i = 0; i < padding; i++)
            {
                func('0');
                length++;
            }
            func('0');
            length++;
            return;
        }
        uint64_t mask = 0xf;
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t digit = (val >> ((15 - i) * 4)) & mask;
            if(digit || i >= 16 - padding)
            {
                func(hex[digit]);
                length++;
            }
        }
    }
    void printf_X(uint64_t val, uint8_t padding)
    {
        if(val == 0)
        {
            for(uint8_t i = 0; i < padding; i++)
            {
                func('0');
                length++;
            }
            func('0');
            length++;
            return;
        }
        uint64_t mask = 0xf;
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t digit = (val >> ((15 - i) * 4)) & mask;
            if(digit || i >= 16 - padding)
            {
                func(HEX[digit]);
                length++;
            }
        }
    }
    void printf_lf(long double val)
    {
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

        // long double fmodl(long double a, long double b)
        // {
        //     while (a >= b || a < 0)
        //         a += b * (a < 0 ? 1 : -1);
        //     return a;
        // }

        uint64_t k = (uint64_t)val;
        long double p = val - (long double)k;
        printf_d(k);
        if (p < 1e-10)
            return;
        func('.');
        length++;
        uint64_t mul = 10;
        uint64_t max_mul = 10;
        // long double mul = 10, max_mul = 10;
        long double _p = p;
        uint16_t digits = 0;
        while (_p != 0 && digits < DOUBLE_PRINT_MAX_DIGITS)
        {
            uint8_t digit = (uint8_t)((uint64_t)(p * mul) % 10);
            _p -= digit / (long double)max_mul;
            max_mul *= 10;
            digits++;
        }
        while(mul < max_mul)
        {
            uint8_t digit = (uint8_t)((uint64_t)(p * mul) % 10); // ((uint8_t)fmodl(p * mul, 10));
            // p -= digit / (long double)max_mul;  // To optimize the fmodl call
            func('0' + digit);
            length++;
            mul *= 10;
        }
    }
    void printf_f(double val)
    {
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

        uint64_t k = (uint64_t)val;
        double p = val - (double)k;
        printf_d(k);
        if (p < 1e-10)
            return;
        func('.');
        length++;
        uint64_t mul = 10;
        uint64_t max_mul = 10;
        double _p = p;
        uint8_t digits = 0;
        while (_p != 0 && digits < FLOAT_PRINT_MAX_DIGITS)
        {
            uint8_t digit = ((uint8_t)((uint64_t)(p * mul) % 10));
            _p -= digit / (long double)max_mul;
            max_mul *= 10;
            digits++;
        }
        while(mul < max_mul)
        {
            uint8_t digit = ((uint8_t)((uint64_t)(p * mul) % 10));
            func('0' + digit);
            length++;
            mul *= 10;
        }
    }

    bool next_arg_64 = false, na64_set = false;
    bool next_formatted = false;
    while (*format)
    {
        if (*format == '%')
        {
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
                    printf_x(va_arg(args, uint64_t), na64_set ? va_arg(args, uint32_t) : 1);
                else
                    printf_x(va_arg(args, uint32_t), na64_set ? va_arg(args, uint32_t) : 1);
                break;
            case 'X':
                if (next_arg_64)
                    printf_X(va_arg(args, uint64_t), na64_set ? va_arg(args, uint32_t) : 1);
                else
                    printf_X(va_arg(args, uint32_t), na64_set ? va_arg(args, uint32_t) : 1);
                break;
            case 'f':
                if (next_arg_64)
                    printf_lf(va_arg(args, long double));
                else
                    printf_f(va_arg(args, double));
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

int fprintf(FILE* stream, const char* format, ...)
{
    void _fputc(char c)
    {
        fputc(c, stream);
    }
    void _fputs(char* s)
    {
        fputs(s, stream);
    }

    va_list args;
    va_start(args, format);
    int length = _printf(_fputc, _fputs, format, args);
    va_end(args);
    return length;
}