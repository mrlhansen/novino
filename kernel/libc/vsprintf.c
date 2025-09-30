#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>

#define LEFT      (1 << 0)
#define PLUS      (1 << 1)
#define MINUS     (1 << 2)
#define SPACE     (1 << 3)
#define SPECIAL   (1 << 4)
#define ZEROPAD   (1 << 5)
#define PRECISION (1 << 6)

static void itoa(uint64_t value, char *buffer, int base)
{
    char *list = "0123456789abcdef";
    int i = 0;

    do
    {
        buffer[i++] = list[value % base];
        value = value / base;
    }
    while(value > 0);

    buffer[i] = '\0';
    strrev(buffer);
}

static void int_convert(char *buf, uint64_t num, int base, char *leading, int flags, int width, int precision)
{
    char sign = 0;
    int len = 0;
    char temp[64];

    itoa(num, temp, base);
    len = strlen(temp);

    if(flags & PRECISION)
    {
        precision -= len;
        flags &= ~ZEROPAD;
        if(precision > 0)
        {
            width -= precision;
        }
    }

    if(flags & SPECIAL)
    {
        width -= strlen(leading);
    }

    if(flags & PLUS)
    {
        sign = '+';
    }

    if(flags & MINUS)
    {
        sign = '-';
    }

    if(sign)
    {
        width--;
    }

    if(flags & ZEROPAD)
    {
        if(sign)
        {
            *buf++ = sign;
            sign = 0;
        }

        if(flags & SPECIAL)
        {
            strcpy(buf, leading);
            buf += strlen(leading);
            flags ^= SPECIAL;
            width += strlen(leading);
        }

        while(width-- > len)
        {
            *buf++ = '0';
        }
    }
    else if((flags & LEFT) == 0)
    {
        while(width-- > len)
        {
            *buf++ = ' ';
        }
    }

    if(sign)
    {
        *buf++ = sign;
    }

    if(flags & SPECIAL)
    {
        strcpy(buf, leading);
        buf += strlen(leading);
    }

    while(precision > 0)
    {
        *buf++ = '0';
        precision--;
    }

    strcpy(buf, temp);
    buf += len;

    while(width > len)
    {
        *buf++ = ' ';
        width--;
    }

    *buf = '\0';
}

static void str_convert(char *buf, int flags, int width, int precision, char *str)
{
    int len;
    len = strlen(str);

    if(flags & PRECISION)
    {
        if(precision < len)
        {
            len = precision;
        }
    }

    if((flags & LEFT) == 0)
    {
        while(width > len)
        {
            *buf++ = ' ';
            width--;
        }
    }

    strncpy(buf, str, len);
    buf += len;

    while(width > len)
    {
        *buf++ = ' ';
        width--;
    }

    *buf = '\0';
}

int vsprintf(char *str, const char *fmt, va_list args)
{
    uint64_t number;
    char *buf = str;
    char temp[128];

    int flags;
    int width;
    int precision;
    int length;

    while(*fmt)
    {
        if(*fmt == '%')
        {
            // Reset number
            number = 0;

            // Determine flags
            flags = 0;
            while(fmt++)
            {
                if(*fmt == '-')
                {
                    flags |= LEFT;
                }
                else if(*fmt == '+')
                {
                    flags |= PLUS;
                }
                else if(*fmt == ' ')
                {
                    flags |= SPACE;
                }
                else if(*fmt == '#')
                {
                    flags |= SPECIAL;
                }
                else if(*fmt == '0')
                {
                    flags |= ZEROPAD;
                }
                else
                {
                    break;
                }
            }

            // Determine width
            width = 0;
            if(isdigit(*fmt))
            {
                while(isdigit(*fmt))
                {
                    width = 10*width + ctoi(*fmt++);
                }
            }
            else if(*fmt == '*')
            {
                width = va_arg(args, int);
                if(width < 0)
                {
                    width = -width;
                    flags |= LEFT;
                }
                fmt++;
            }

            // Determine precision
            precision = 0;
            if(*fmt == '.')
            {
                fmt++;
                flags |= PRECISION;
                if(isdigit(*fmt))
                {
                    while(isdigit(*fmt))
                    {
                        precision = 10*precision + ctoi(*fmt++);
                    }
                }
                else if(*fmt == '*')
                {
                    precision = va_arg(args, int);
                    if(precision < 0)
                    {
                        precision = 0;
                    }
                    fmt++;
                }
            }

            // Determine length
            length = 0;
            if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
            {
                length = *fmt;
                fmt++;
            }

            // Handle signed integers
            if(*fmt == 'd' || *fmt == 'i')
            {
                if(length == 'l')
                {
                    int64_t tmp = va_arg(args, int64_t);
                    if(tmp < 0)
                    {
                        tmp = -tmp;
                        flags |= MINUS;
                    }
                    number = tmp;
                }
                else
                {
                    int32_t tmp = va_arg(args, int32_t);
                    if(tmp < 0)
                    {
                        tmp = -tmp;
                        flags |= MINUS;
                    }
                    number = tmp;
                }
            }

            // Handle unsigned integers
            if(*fmt == 'u' || *fmt == 'x' || *fmt == 'X' || *fmt == 'o')
            {
                if(length == 'l')
                {
                    number = va_arg(args, uint64_t);
                }
                else
                {
                    number = va_arg(args, uint32_t);
                }
            }

            // Handle specifier
            switch(*fmt)
            {
                case 'd':
                case 'i':
                    int_convert(temp, number, 10, "", flags, width, precision);
                    strcpy(buf, temp);
                    buf += strlen(temp);
                    break;

                case 'u':
                    int_convert(temp, number, 10, "", flags, width, precision);
                    strcpy(buf, temp);
                    buf += strlen(temp);
                    break;

                case 'x':
                    int_convert(temp, number, 16, "0x", flags, width, precision);
                    strcpy(buf, temp);
                    buf += strlen(temp);
                    break;

                case 'X':
                    int_convert(temp, number, 16, "0x", flags, width, precision);
                    strtoupper(temp);
                    strcpy(buf, temp);
                    buf += strlen(temp);
                    break;

                case 'c':
                    temp[0] = va_arg(args, int);
                    temp[1] = '\0';
                    str_convert(buf, flags, width, precision, temp);
                    buf += strlen(buf);
                    break;

                case 'o':
                    int_convert(temp, number, 8, "0", flags, width, precision);
                    strcpy(buf, temp);
                    buf += strlen(temp);
                    break;

                case 's':
                    str_convert(buf, flags, width, precision, va_arg(args, char*));
                    buf += strlen(buf);
                    break;

                case '%':
                    *buf++ = '%';
                    break;

                case '\0':
                    continue;

                default:
                    *buf++ = *fmt;
                    break;
            }
        }
        else
        {
            *buf++ = *fmt;
        }

        fmt++;
    }

    *buf = '\0';
    return (int)(buf - str);
}
