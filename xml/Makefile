CC=gcc
CFLAGS=-g -I/usr/local/DmsInclude -L/usr/local/DmsLib
DLLFLAGS= -lxml2
EOBJ=./xml
SOURCE=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SOURCE))
$(EOBJ):$(OBJS)
	C_INCLUDE_PATH=/usr/include/libxml2:/MyLib
	export C_INCLUDE_PATH
	$(CC) $(CFLAGS) $(OBJS) $(DLLFLAGS) -o $(EOBJ)
clean:
	rm -f $(OBJS) $(EOBJ)
