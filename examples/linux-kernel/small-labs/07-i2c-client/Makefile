KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += lab_i2c.o
obj-m += lab_i2c_client.o

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

