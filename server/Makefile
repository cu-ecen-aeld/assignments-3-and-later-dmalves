CC=gcc
CFLAGS=-I. -Wall -Werror -g -O0
DEPS = 
OBJ = aesdsocket.o  become_daemon.o

default: aesdsocket
all: aesdsocket

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

aesdsocket: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o aesdsocket *.c~
