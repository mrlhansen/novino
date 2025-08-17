#ifndef STDINT_H
#define STDINT_H

/* integral types */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long int64_t;
typedef unsigned long uint64_t;

/* least types */
typedef signed char int_least8_t;
typedef unsigned char uint_least8_t;
typedef signed short int_least16_t;
typedef unsigned short uint_least16_t;
typedef signed int int_least32_t;
typedef unsigned int uint_least32_t;
typedef signed long int_least64_t;
typedef unsigned long uint_least64_t;

/* fast types */
typedef signed char int_fast8_t;
typedef unsigned char uint_fast8_t;
typedef signed short int_fast16_t;
typedef unsigned short uint_fast16_t;
typedef signed int int_fast32_t;
typedef unsigned int uint_fast32_t;
typedef signed long int_fast64_t;
typedef unsigned long uint_fast64_t;

/* types for void* pointers */
typedef long intptr_t;
typedef unsigned long uintptr_t;

/* largest integral types */
typedef long intmax_t;
typedef unsigned long uintmax_t;

/* minimum value of signed integral types */
#define INT8_MIN  (-128)
#define INT16_MIN (-32768)
#define INT32_MIN (-2147483648)
#define INT64_MIN (-9223372036854775808L)

/* maximum value of signed integral types */
#define INT8_MAX  (127)
#define INT16_MAX (32767)
#define INT32_MAX (2147483647)
#define INT64_MAX (9223372036854775807L)

/* maximum value of unsigned integral types */
#define UINT8_MAX  (255)
#define UINT16_MAX (65535)
#define UINT32_MAX (4294967295U)
#define UINT64_MAX (18446744073709551615UL)

/* minimum value of signed least types */
#define INT_LEAST8_MIN  (-128)
#define INT_LEAST16_MIN (-32768)
#define INT_LEAST32_MIN (-2147483648)
#define INT_LEAST64_MIN (-9223372036854775808L)

/* maximum value of signed least types */
#define INT_LEAST8_MAX  (127)
#define INT_LEAST16_MAX (32767)
#define INT_LEAST32_MAX (2147483647)
#define INT_LEAST64_MAX (9223372036854775807L)

/* maximum value of unsigned least types */
#define UINT_LEAST8_MAX  (255)
#define UINT_LEAST16_MAX (65535)
#define UINT_LEAST32_MAX (4294967295U)
#define UINT_LEAST64_MAX (18446744073709551615UL)

/* minimum value of signed fast types */
#define INT_FAST8_MIN  (-128)
#define INT_FAST16_MIN (-32768)
#define INT_FAST32_MIN (-2147483648)
#define INT_FAST64_MIN (-9223372036854775808L)

/* maximum value of signed fast types */
#define INT_FAST8_MAX  (127)
#define INT_FAST16_MAX (32767)
#define INT_FAST32_MAX (2147483647)
#define INT_FAST64_MAX (9223372036854775807L)

/* maximum value of unsigned fast types */
#define UINT_FAST8_MAX  (255)
#define UINT_FAST16_MAX (65535)
#define UINT_FAST32_MAX (4294967295U)
#define UINT_FAST64_MAX (18446744073709551615UL)

/* minimum and maximum values for void* pointers */
#define INTPTR_MIN  (-9223372036854775808L)
#define INTPTR_MAX  (9223372036854775807L)
#define UINTPTR_MAX (18446744073709551615UL)

/* minimum and maximum values largest integral types */
#define INTMAX_MIN  (-9223372036854775808L)
#define INTMAX_MAX  (9223372036854775807L)
#define UINTMAX_MAX (18446744073709551615UL)

/* signed */
#define INT8_C(c)  c
#define INT16_C(c) c
#define INT32_C(c) c
#define INT64_C(c) c ## L

/* unsigned */
#define UINT8_C(c)  c
#define UINT16_C(c) c
#define UINT32_C(c) c ## U
#define UINT64_C(c) c ## UL

/* maximal type */
#define INTMAX_C(c)  c ## L
#define UINTMAX_C(c) c ## UL

/* limit of size_t */
#define SIZE_MAX (18446744073709551615UL)

#endif
