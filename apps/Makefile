SUBDIRS := init yash cat ls mount umount stat mkdir rmdir rm
SOURCES := $(shell find $(SUBDIRS) -name '*.c')
OBJECTS := $(SOURCES:.c=.o)
TARGETS := $(SUBDIRS:=.elf)

all: $(OBJECTS) $(TARGETS)

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

%.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -T user.ld -o $@ $(@:.elf=)/*.o -lc

clean:
	rm -f $(OBJECTS) $(TARGETS)
