SOURCES := crti.S crtn.S
OBJECTS := crti.o crtbegin.o crtend.o crtn.o

all: $(OBJECTS)

%.o: %.S
	$(CC) $(CCFLAGS) -c $< -o $@

crtbegin.o crtend.o:
	cp $(shell $(CC) $(CCFLAGS) -print-file-name=$@) $@

clean:
	rm -f $(OBJECTS)
