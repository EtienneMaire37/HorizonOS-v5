int fputc(int c, FILE* stream)
{
    asm("int 0xff" : 
        : "a" (1), "b" ((char)c), "c" (stream));
    return c;
}

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
    fputs(s, stdout);
}

FILE* fopen(const char* path, const char* mode)
{
    return NULL;
}

int fclose(FILE* stream)
{
    return 0;
}

int fprintf(FILE* stream, const char* format, ...)
{
    char hex[16] = "0123456789abcdef";
    char HEX[16] = "0123456789ABCDEF";

    void printf_d(int64_t val)
    {
        if(val < 0)
        {
            fputc('-', stream);
            val = -val;
        }
        if(val == 0)
        {
            fputc('0', stream);
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
                fputc('0' + digit, stream);
            div /= 10;
        }
    }
    void printf_u(uint64_t val)
    {
        if(val == 0)
        {
            fputc('0', stream);
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
                fputc('0' + digit, stream);
            div /= 10;
        }
    }
    void printf_x(uint64_t val, uint8_t padding)
    {
        if(val == 0)
        {
            for(uint8_t i = 0; i < padding; i++)
                fputc('0', stream);
            fputc('0', stream);
            return;
        }
        uint64_t mask = 0xf;
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t digit = (val >> ((15 - i) * 4)) & mask;
            if(digit || i >= 16 - padding)
                fputc(hex[digit], stream);
        }
    }
    void printf_X(uint64_t val, uint8_t padding)
    {
        if(val == 0)
        {
            for(uint8_t i = 0; i < padding; i++)
                fputc('0', stream);
            fputc('0', stream);
            return;
        }
        uint64_t mask = 0xf;
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t digit = (val >> ((15 - i) * 4)) & mask;
            if(digit || i >= 16 - padding)
                fputc(HEX[digit], stream);
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
            case 'l':
                na64_set = true;
                break;
            case 's':
                fputs(va_arg(args, char*), stream);
                break;
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
            fputc(*format, stream);
        format++;
    }
}