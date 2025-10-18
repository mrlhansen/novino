#include <_stdio.h>
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
#define UPPERCASE (1 << 7)

static void _fputc(int ch, FILE *fp)
{
    if(fp->fd < 0)
    {
        if(fp->b.pos < fp->b.end)
        {
            *fp->b.pos++ = ch;
        }
    }
    else
    {
        fputc(ch, fp);
    }
}

static void _fputs(const char *str, FILE *fp)
{
    while(*str)
    {
        _fputc(*str, fp);
        str++;
    }
}

static void itoa(size_t value, char *buffer, int base)
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

static int int_convert(FILE *fp, size_t num, int base, char *leading, int flags, int width, int precision)
{
    int len, rlen, slen;
    char sign = 0;
    char temp[64];

    itoa(num, temp, base);
    if(flags & UPPERCASE)
    {
        strtoupper(temp);
    }
    len = strlen(temp);
    slen = strlen(leading);
    rlen = 0;

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
            _fputc(sign, fp);
            rlen++;
            sign = 0;
        }

        if(flags & SPECIAL)
        {
            if(slen)
            {
                _fputs(leading, fp);
                rlen += slen;
                width += slen;
            }
            flags ^= SPECIAL;
        }

        while(width > len)
        {
            _fputc('0', fp);
            rlen++;
            width--;
        }
    }
    else if((flags & LEFT) == 0)
    {
        while(width > len)
        {
            _fputc(' ', fp);
            rlen++;
            width--;
        }
    }

    if(sign)
    {
        _fputc(sign, fp);
        rlen++;
    }

    if(flags & SPECIAL)
    {
        _fputs(leading, fp);
        rlen += slen;
    }

    while(precision > 0)
    {
        _fputc('0', fp);
        rlen++;
        precision--;
    }

    _fputs(temp, fp);
    rlen += len;

    while(width > len)
    {
        _fputc(' ', fp);
        rlen++;
        width--;
    }

    return rlen;
}

static int str_convert(FILE *fp, int flags, int width, int precision, char *str)
{
    int rlen, len;

    len = strlen(str);

    if(flags & PRECISION)
    {
        if(precision < len)
        {
            len = precision;
        }
    }

    rlen = len;

    if((flags & LEFT) == 0)
    {
        while(width > len)
        {
            _fputc(' ', fp);
            rlen++;
            width--;
        }
    }

    for(int i = 0; i < len; i++)
    {
        _fputc(str[i], fp);
    }

    while(width > len)
    {
        _fputc(' ', fp);
        rlen++;
        width--;
    }

    return rlen;
}

int vfprintf(FILE *fp, const char *fmt, va_list args)
{
    size_t number;
    char temp[2];
    int rlen = 0;
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
                    long tmp = va_arg(args, long);
                    if(tmp < 0)
                    {
                        tmp = -tmp;
                        flags |= MINUS;
                    }
                    number = tmp;
                }
                else
                {
                    int tmp = va_arg(args, int);
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
                    number = va_arg(args, unsigned long);
                }
                else
                {
                    number = va_arg(args, unsigned int);
                }
            }

            // Handle specifier
            switch(*fmt)
            {
                case 'd':
                case 'i':
                    rlen += int_convert(fp, number, 10, "", flags, width, precision);
                    break;
                case 'u':
                    rlen += int_convert(fp, number, 10, "", flags, width, precision);
                    break;
                case 'x':
                    rlen += int_convert(fp, number, 16, "0x", flags, width, precision);
                    break;
                case 'X':
                    flags |= UPPERCASE;
                    rlen += int_convert(fp, number, 16, "0x", flags, width, precision);
                    break;
                case 'c':
                    temp[0] = va_arg(args, int);
                    temp[1] = '\0';
                    rlen += str_convert(fp, flags, width, precision, temp);
                    break;
                case 'o':
                    rlen += int_convert(fp, number, 8, "0", flags, width, precision);
                    break;
                case 's':
                    rlen += str_convert(fp, flags, width, precision, va_arg(args, char*));
                    break;
                case '%':
                    _fputc('%', fp);
                    rlen++;
                    break;
                case '\0':
                    continue;
                default:
                    _fputc(*fmt, fp);
                    rlen++;
                    break;
            }
        }
        else
        {
            _fputc(*fmt, fp);
            rlen++;
        }

        fmt++;
    }

    return rlen;
}
