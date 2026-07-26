KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += platform_demo.o
obj-m += platform_demo_device.o

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

