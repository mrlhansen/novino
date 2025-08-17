#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// consume chars from "fp" until "width" reaches zero, then simply return \0 afterwards
static inline char consume(FILE *fp, int *width)
{
    int ch = '\0';
    if(*width)
    {
        ch = fgetc(fp);
        (*width)--;
    }
    return ch;
}

// Converts a string to an integer
// The integer is signed when "sret" is set and unsigned when "uret" is set
// At most "width" bytes are read from the input string (not including leading white spaces)
// "base" determines the base for the conversion (binary, octal, decimal, hex are supported)
// When "base" is zero, the base is inferred from the prefix, using this list:
// Binary: 0b, 0B
// Octal: 0, 0o, 0O
// Decimal: default when no prefix
// Hex: 0x, 0X
static int int_convert(FILE *fp, int width, int base, ssize_t *sret, size_t *uret)
{
    size_t res = 0;
    int autobase = 0;
    int sign = 1;
    int ch, max;

    // width = 0 means unlimited
    if(width == 0)
    {
        width = 128;
    }

    // skip whitespaces
    do {
        ch = fgetc(fp);
    } while(isspace(ch));

    // consumed a non-whitespace char above
    width--;

    // check sign
    if(ch == '-')
    {
        sign = -1;
        ch = consume(fp, &width);
    }
    else if(ch == '+')
    {
        ch = consume(fp, &width);
    }

    if(uret && sign < 0)
    {
        return -1; // ungetc??
    }

    // detect base
    if(ch == '0')
    {
        ch = consume(fp, &width);
        if(ch == 'x' || ch == 'X')
        {
            autobase = 16;
            ch = consume(fp, &width);
        }
        else if(ch == 'o' || ch == 'O')
        {
            autobase = 8;
            ch = consume(fp, &width);
        }
        else if(ch >= '0' && ch <= '7' && base == 0)
        {
            autobase = 8;
        }
        else if(ch == 'b' || ch == 'B')
        {
            autobase = 2;
            ch = consume(fp, &width);
        }
    }

    if(base)
    {
        if(autobase && autobase != base)
        {
            return -1; // ungetc??
        }
    }
    else
    {
        base = autobase ? autobase : 10;
    }

    // Parse number
    if(base == 16)
    {
        while(isxdigit(ch))
        {
            res = res*base + ctoi(ch);
            ch = consume(fp, &width);
        }
    }
    else
    {
        max = '0' + base - 1;
        while(ch >= '0' && ch <= max)
        {
            res = res*base + ctoi(ch);
            ch = consume(fp, &width);
        }
    }

    // return value
    if(sret)
    {
        *sret = sign*res;
    }
    else
    {
        *uret = res;
    }

    // unget last char
    if(ch)
    {
        ungetc(ch, fp);
    }

    return 0;
}

// takes a sequence of the form [abc] or [^abc] and checks if the input char is accepted
// "s" is the start of the sequence ([ char) and "e" is the end of the sequence (] char)
static int isvalid(int ch, const char *s, const char *e)
{
    int neg = 0;
    int found = 0;

    // negated list?
    s++;
    if(*s == '^')
    {
        neg = 1;
        s++;
    }

    // scan sequences
    while(s < e)
    {
        if(ch == *s)
        {
            found = 1;
            break;
        }
        s++;
    }

    // return result
    if(neg && !found)
    {
        return ch;
    }

    if(!neg && found)
    {
        return ch;
    }

    return 0;
}

// copy a char sequence as long as the sequence is valid
// "s" and "e" is the beginning and the end of an optional sequence of valid chars
// when no sequence is given, we stop at the next whitespace or at the end of the string
// at most "width" chars are copied into the destination buffer
static void str_convert(FILE *fp, char *s, char *e, int width, char *dest)
{
    int ch;

    // width = 0 means unlimited
    if(width == 0)
    {
        width = 4096;
    }

    // skip whitespaces
    ch = fgetc(fp);
    if(!s)
    {
        while(isspace(ch))
        {
            ch = fgetc(fp);
        }
    }

    // start parsing
    while(width && ch)
    {
        if(s)
        {
            if(!isvalid(ch, s, e))
            {
                break;
            }
        }
        else
        {
            if(isspace(ch))
            {
                break;
            }
        }

        if(dest)
        {
            *dest++ = ch;
            *dest = '\0';
        }

        width--;
        ch = fgetc(fp);
    }

    // unget last char
    ungetc(ch, fp);
}

int vfscanf(FILE *fp, const char *fmt, va_list args)
{
    int nargs = 0;
    char *s, *e;
    ssize_t sval;
    size_t uval;
    int length;
    int suppressed;
    int width;
    int assign;
    int ret, ch;

    while(*fmt)
    {
        // consume whitespace
        if(isspace(*fmt))
        {
            do {
                ch = fgetc(fp);
            } while(isspace(ch));
            ungetc(ch, fp);

            while(isspace(*fmt))
            {
                fmt++;
            }
        }

        // parse input
        if(*fmt == '%')
        {
            fmt++;

            // suppress assignment
            suppressed = 0;
            if(*fmt == '*')
            {
                suppressed = 1;
                fmt++;
            }

            // maximum field width
            width = 0;
            while(isdigit(*fmt))
            {
                width = 10*width + (*fmt++ - '0');
            }

            // length modifier
            length = 0;
            if(*fmt == 'l')
            {
                length = *fmt;
                fmt++;
            }

            // handle sequences
            if(*fmt == '[')
            {
                s = (char*)fmt;
                while(*fmt)
                {
                    if(*fmt == ']')
                    {
                        break;
                    }
                    fmt++;
                }
                if(*fmt == '\0')
                {
                    break;
                }
                e = (char*)fmt;
            }
            else
            {
                s = 0;
                e = 0;
            }

            sval = 0;
            uval = 0;
            assign = 0;

            switch(*fmt)
            {
                case 'i':
                    ret = int_convert(fp, width, 0, &sval, 0);
                    assign = 1;
                    fmt++;
                    break;
                case 'd':
                    ret = int_convert(fp, width, 10, &sval, 0);
                    assign = 1;
                    fmt++;
                    break;
                case 'b':
                    ret = int_convert(fp, width, 2, 0, &uval);
                    assign = 1;
                    fmt++;
                    break;
                case 'o':
                    ret = int_convert(fp, width, 8, 0, &uval);
                    assign = 1;
                    fmt++;
                    break;
                case 'u':
                    ret = int_convert(fp, width, 10, 0, &uval);
                    assign = 1;
                    fmt++;
                    break;
                case 'x':
                    ret = int_convert(fp, width, 16, 0, &uval);
                    assign = 1;
                    fmt++;
                    break;
                case 'c':
                    if(!suppressed)
                    {
                        *va_arg(args, char*) = fgetc(fp);
                        nargs++;
                    }
                    fmt++;
                    break;
                case ']':
                case 's':
                    if(suppressed)
                    {
                        str_convert(fp, s, e, width, 0);
                    }
                    else
                    {
                        str_convert(fp, s, e, width, va_arg(args, char*));
                        nargs++;
                    }
                    fmt++;
                    break;
                case '%':
                    ch = fgetc(fp);
                    if(ch == *fmt)
                    {
                        fmt++;
                    }
                    else
                    {
                        ungetc(ch, fp);
                    }
                    break;
                default:
                    break;
            }

            if(assign)
            {
                if(ret < 0)
                {
                    break;
                }

                if(!suppressed)
                {
                    if(sval)
                    {
                        if(length == 'l')
                        {
                            *va_arg(args, long*) = sval;
                            nargs++;
                        }
                        else
                        {
                            *va_arg(args, int*) = sval;
                            nargs++;
                        }
                    }
                    else
                    {
                        if(length == 'l')
                        {
                            *va_arg(args, unsigned long*) = uval;
                            nargs++;
                        }
                        else
                        {
                            *va_arg(args, unsigned int*) = uval;
                            nargs++;
                        }
                    }
                }
            }

            continue;
        }

        ch = fgetc(fp);
        if(ch == *fmt)
        {
            fmt++;
        }
        else
        {
            ungetc(ch, fp);
            break;
        }
    }

    return nargs;
}
