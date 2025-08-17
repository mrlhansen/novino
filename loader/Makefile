SUBDIRS := multiboot2
OBJECTS := $(shell find $(SUBDIRS) -name '*.o')
TARGETS := kernel.bin

all: $(TARGETS)

.PHONY: kernel.bin
kernel.bin:
	$(CC) $(LDFLAGS) -T loader.ld -o $@ $(OBJECTS) -lc

.PHONY: clean
clean:
	rm -f $(TARGETS)
