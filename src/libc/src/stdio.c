// int fputc(int c, FILE* stream)
// {
//     asm("int 0xff" : 
//         : "a" (1), "b" ((char)c), "c" (stream));
//     return c;
// }

int putchar(int c)
{
    return fputc(c, stdout);
}

int fputs(const char* s, FILE* stream)
{
    while(*s)
        fputc(*s++, stream);
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

#define FLOAT_PRINT_MAX_DIGITS 10

int fprintf(FILE* stream, const char* format, ...)
{
    char hex[16] = "0123456789abcdef";
    char HEX[16] = "0123456789ABCDEF";

    uint32_t length = 0;

    void printf_d(int64_t val)
    {
        if(val < 0)
        {
            fputc('-', stream);
            length++;
            val = -val;
        }
        if(val == 0)
        {
            fputc('0', stream);
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
                fputc('0' + digit, stream);
                length++;
            }
            div /= 10;
        }
    }
    void printf_u(uint64_t val)
    {
        if(val == 0)
        {
            fputc('0', stream);
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
                fputc('0' + digit, stream);
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
                fputc('0', stream);
                length++;
            }
            fputc('0', stream);
            length++;
            return;
        }
        uint64_t mask = 0xf;
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t digit = (val >> ((15 - i) * 4)) & mask;
            if(digit || i >= 16 - padding)
            {
                fputc(hex[digit], stream);
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
                fputc('0', stream);
                length++;
            }
            fputc('0', stream);
            length++;
            return;
        }
        uint64_t mask = 0xf;
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t digit = (val >> ((15 - i) * 4)) & mask;
            if(digit || i >= 16 - padding)
            {
                fputc(HEX[digit], stream);
                length++;
            }
        }
    }
    void printf_lf(long double val)
    {
        int64_t k = (int64_t)val;
        long double p = val - (long double)k;
        printf_d(k);
        fputc('.', stream);
        length++;
        uint64_t mul = 10;
        uint64_t max_mul = 10;
        long double _p = p;
        uint8_t digits = 0;
        while (_p != 0 && digits < FLOAT_PRINT_MAX_DIGITS)
        {
            _p -= (((uint8_t)(max_mul * _p)) % 10) / (long double)max_mul;
            max_mul *= 10;
            digits++;
        }
        while(mul < max_mul)
        {
            uint8_t digit = ((uint8_t)(p * mul)) % 10;
            fputc('0' + digit, stream);
            length++;
            mul *= 10;
        }
    }
    void printf_f(double val)
    {
        int64_t k = (int64_t)val;
        double p = val - (double)k;
        printf_d(k);
        fputc('.', stream);
        length++;
        uint64_t mul = 10;
        uint64_t max_mul = 10;
        double _p = p;
        uint8_t digits = 0;
        while (_p != 0 && digits < FLOAT_PRINT_MAX_DIGITS)
        {
            _p -= (((uint8_t)(max_mul * _p)) % 10) / (double)max_mul;
            max_mul *= 10;
            digits++;
        }
        while(mul < max_mul)
        {
            uint8_t digit = ((uint8_t)(p * mul)) % 10;
            fputc('0' + digit, stream);
            length++;
            mul *= 10;
        }
    }

    va_list args;
    va_start(args, format);
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
            // {
            //     char* s = va_arg(args, char*);
            //     fputs(s, stream);
            //     length += strlen(s);
            //     break;
            // }
            {
                char* s = va_arg(args, char*);
                while(*s)
                {
                    fputc(*s++, stream);
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
            fputc(*format, stream);
            length++;
        }
        format++;
    }

    va_end(args);

    return length;
}