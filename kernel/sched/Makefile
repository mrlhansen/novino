SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o) spinlock.o ring3.o

all: $(OBJECTS)

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

%.o: %.asm
	$(NASM) -f elf64 $< -o $@

clean:
	rm -f $(OBJECTS)
