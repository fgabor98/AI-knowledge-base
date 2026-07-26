KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
DTC ?= dtc

obj-m += dt_demo.o

.PHONY: all modules clean

all: modules demo-overlay.dtbo

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

demo-overlay.dtbo: demo-overlay.dts
	$(DTC) -@ -I dts -O dtb -o $@ $<

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f demo-overlay.dtbo

