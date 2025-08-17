SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o) apboot.bin aplink.o interrupts.o

all: $(OBJECTS)

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

%.o: %.asm
	$(NASM) -f elf64 $< -o $@

%.bin: %.asm
	$(NASM) -f bin $< -o $@

clean:
	rm -f $(OBJECTS)
