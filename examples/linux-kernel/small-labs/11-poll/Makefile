KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += lab_poll.o

.PHONY: all modules clean

all: modules test_poll

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

test_poll: test_poll.c
	$(CC) $(TEST_CFLAGS) -std=gnu11 -Wall -Wextra -O2 -o $@ $<

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f test_poll
