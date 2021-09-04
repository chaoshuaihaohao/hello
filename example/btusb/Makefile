# Specify flags for the module compilation.
EXTRA_CFLAGS=-g -O1

#fix warning:
#[  524.130936] btusb: loading out-of-tree module taints kernel.
#[  524.131006] btusb: module verification failed: signature and/or required key missing - tainting kernel
CONFIG_MODULE_SIG=n

#include current dir's *.h
srctree := ./
#ccflags += -I.
# If KERNELRELEASE is defined， we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
obj-m := btusb.o
# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
KERNELDIR ?=/lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
endif
