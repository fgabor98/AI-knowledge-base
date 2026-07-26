KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
DTC ?= dtc

obj-m += gpio_demo.o

.PHONY: all modules clean

all: modules gpio-demo-overlay.dtbo

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

gpio-demo-overlay.dtbo: gpio-demo-overlay.dts
	$(DTC) -@ -I dts -O dtb -o $@ $<

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f gpio-demo-overlay.dtbo

