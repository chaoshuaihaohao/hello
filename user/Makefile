CFLAGS += -fstack-protector -fPIE -Wframe-larger-than=2048
CFLAGS += -g -O0 --std=gnu11 -Wall -Wextra -Wuninitialized #-Werror -fstack-protector -fPIE
CFLAGS += -Wno-unused-parameter -Wno-sign-compare

YSFMEMGR_SRC = acl.c

acl: clean
	gcc -o $@ $(YSFMEMGR_SRC) $(CFLAGS)

all: acl

clean:
	rm -f *.o ysfmemgr acl

install:
	mkdir -p /tmp/user
	cp etc/table* /tmp/user/
