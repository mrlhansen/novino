SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o)
TARGETS := libsabi.a

all: $(TARGETS)

libsabi.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)
	cp -p $@ $(SYSROOT)$(LIBDIR)

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGETS)
