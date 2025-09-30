EXCLUDE := crt
SUBDIRS := $(subst /,,$(wildcard */))
SUBDIRS := $(filter-out $(EXCLUDE),$(SUBDIRS))
OBJECTS := $(shell find $(SUBDIRS) -name '*.o')
TARGETS := kernel.elf

all: $(TARGETS)

.PHONY: kernel.elf
kernel.elf:
	$(CC) $(LDFLAGS) -T kernel.ld -o $@ crt/crti.o crt/crtbegin.o $(OBJECTS) -lgcc -lsabi crt/crtend.o crt/crtn.o

.PHONY: clean
clean:
	rm -f $(TARGETS)
