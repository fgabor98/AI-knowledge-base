# Lab 2: Module Parameters

This checkpoint passes a small, validated configuration value when the module
is loaded and exposes it read-only through the module parameter sysfs file.

```sh
make
sudo insmod ./lab_params.ko interval_ms=500
cat /sys/module/lab_params/parameters/interval_ms
dmesg | tail -n 20
sudo rmmod lab_params
```

The value must be between 1 and 60000. Try an invalid value and observe that
module initialization fails:

```sh
sudo insmod ./lab_params.ko interval_ms=0
```

This is intentionally a module parameter rather than hardware description.
Board wiring belongs in firmware data such as Device Tree.

