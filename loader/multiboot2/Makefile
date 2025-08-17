SOURCES := main.c stub.asm
OBJECTS := main.o stub.o

CCFLAGS := $(CCFLAGS)

all: clean $(OBJECTS)

%.o: %.asm
	$(NASM) -f elf64 $< -o $@

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJECTS)
