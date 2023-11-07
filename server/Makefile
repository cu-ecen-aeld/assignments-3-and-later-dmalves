CC=gcc
CFLAGS=-I.
DEPS = 
OBJ = aesdsocket.o 

default: aesdsocket
all: aesdsocket

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

aesdsocket: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o aesdsocket 
