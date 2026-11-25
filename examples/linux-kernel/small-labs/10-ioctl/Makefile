KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += lab_ioctl.o

.PHONY: all modules clean

all: modules test_ioctl

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

test_ioctl: test_ioctl.c lab_ioctl.h
	$(CC) $(TEST_CFLAGS) -std=gnu11 -Wall -Wextra -O2 -o $@ $<

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f test_ioctl
