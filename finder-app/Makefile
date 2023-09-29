CC=gcc
CFLAGS=-I.
DEPS = 
OBJ = writer.o 

ifdef CROSS_COMPILE
CC=$(CROSS_COMPILE)gcc
endif

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

writer: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o writer
