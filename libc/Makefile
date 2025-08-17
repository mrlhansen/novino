SOURCES := $(shell find . -type f -name '*.c')
OBJECTS := $(SOURCES:.c=.o)
TARGETS := libc.a

all: $(TARGETS)

libc.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)
	cp -p $@ $(SYSROOT)$(LIBDIR)

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGETS)
