# Lab 1: Hello Module

This checkpoint demonstrates module initialization, module cleanup, kernel
logging, and the external-module build loop.

Build and load it on the running kernel:

```sh
make
sudo insmod ./hello.ko
dmesg | tail -n 20
lsmod | grep '^hello '
sudo rmmod hello
dmesg | tail -n 20
```

Expected evidence includes `lab1_hello: loaded` and
`lab1_hello: unloaded`. The module has no hardware dependencies.

