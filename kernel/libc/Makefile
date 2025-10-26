SOURCES := memset.c
SOURCES += memchr.c
SOURCES += memcpy.c
SOURCES += memcmp.c
SOURCES += strlen.c
SOURCES += strcpy.c
SOURCES += strncpy.c
SOURCES += strscpy.c
SOURCES += strtrim.c
SOURCES += strcmp.c
SOURCES += strncmp.c
SOURCES += strcat.c
SOURCES += strtok.c
SOURCES += strchr.c
SOURCES += strstr.c
SOURCES += strrchr.c
SOURCES += strrev.c
SOURCES += strtolower.c
SOURCES += strtoupper.c
SOURCES += tolower.c
SOURCES += toupper.c
SOURCES += isspace.c
SOURCES += isdigit.c
SOURCES += ctoi.c
SOURCES += min.c
SOURCES += max.c
SOURCES += mbtowc.c
SOURCES += gmtime.c
SOURCES += mktime.c
SOURCES += asctime.c
SOURCES += sprintf.c
SOURCES += vsprintf.c
OBJECTS := $(SOURCES:.c=.o)

vpath %.c .
vpath %.c ../../libc/string
vpath %.c ../../libc/stdlib
vpath %.c ../../libc/ctype
vpath %.c ../../libc/stdio
vpath %.c ../../libc/time

all: $(OBJECTS)

%.o : %.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
