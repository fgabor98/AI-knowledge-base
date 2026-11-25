# Lab 16: Exported Symbols

`lab_provider` exports one GPL-only function in the `KB_LAB` symbol namespace;
`lab_consumer` imports and calls it.

```sh
make
modinfo -F depends lab_consumer.ko
modinfo -F import_ns lab_consumer.ko
sudo insmod ./lab_consumer.ko      # fails: Unknown symbol
sudo insmod ./lab_provider.ko
sudo insmod ./lab_consumer.ko
lsmod | grep ^lab_provider         # used by lab_consumer
sudo rmmod lab_provider            # fails: in use
sudo rmmod lab_consumer lab_provider
dmesg | tail
```

`insmod` does not resolve dependencies; `modprobe` would, after
`make -C /lib/modules/$(uname -r)/build M=$PWD modules_install` and `depmod`.

Experiments:

- Remove `MODULE_IMPORT_NS()` and rebuild: modpost reports the missing
  namespace import.
- Change the consumer license to `"Proprietary"`: modpost refuses the
  GPL-only symbol.
- Run `grep KB_LAB Module.symvers` and identify each column.
