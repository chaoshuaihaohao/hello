CC=gcc
CFLAGS=-g -O0 -pthread
DLLFLAGS=
EOBJ=./a.out
SOURCE=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SOURCE))
$(EOBJ):$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(DLLFLAGS) -o $(EOBJ)
clean:
	rm -f $(OBJS) $(EOBJ)
